#include "batch.hpp"

#include <tracy/Tracy.hpp>

#include "../../ui/utils.hpp"

namespace SpriteFlags {
    enum : uint8_t {
        UI = 0,
        World,
        IgnoreCameraZoom,
    };
};

void Batch::DrawSprite(const Sprite& sprite, bool is_ui, Order custom_order) {
    glm::vec4 uv_offset_scale = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);

    if (sprite.flip_x()) {
        uv_offset_scale.x += uv_offset_scale.z;
        uv_offset_scale.z *= -1.0f;
    }

    if (sprite.flip_y()) {
        uv_offset_scale.y += uv_offset_scale.w;
        uv_offset_scale.w *= -1.0f;
    }

    uint32_t order = custom_order.value >= 0 ? custom_order.value : m_order;

    AddSpriteDrawCommand(sprite, uv_offset_scale, sprite.texture(), order, is_ui);

    if (custom_order.advance) m_order = ++order;
    if (!m_order_mode) ++m_sprite_count;
}

void Batch::DrawAtlasSprite(const TextureAtlasSprite& sprite, bool is_ui, Order custom_order) {
    const math::Rect& rect = sprite.atlas().get_rect(sprite.index());

    glm::vec4 uv_offset_scale = glm::vec4(
        rect.min.x / sprite.atlas().texture().width(),
        rect.min.y / sprite.atlas().texture().height(),
        rect.size().x / sprite.atlas().texture().width(),
        rect.size().y / sprite.atlas().texture().height()
    );

    if (sprite.flip_x()) {
        uv_offset_scale.x += uv_offset_scale.z;
        uv_offset_scale.z *= -1.0f;
    }

    if (sprite.flip_y()) {
        uv_offset_scale.y += uv_offset_scale.w;
        uv_offset_scale.w *= -1.0f;
    }

    uint32_t order = custom_order.value >= 0 ? custom_order.value : m_order;

    AddSpriteDrawCommand(sprite, uv_offset_scale, sprite.atlas().texture(), order, is_ui);

    if (custom_order.advance) m_order = ++order;

    ++m_sprite_count;
}

void Batch::DrawNinePatch(const NinePatch& ninepatch, bool is_ui, Order custom_order) {
    glm::vec4 uv_offset_scale = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);

    if (ninepatch.flip_x()) {
        uv_offset_scale.x += uv_offset_scale.z;
        uv_offset_scale.z *= -1.0f;
    }

    if (ninepatch.flip_y()) {
        uv_offset_scale.y += uv_offset_scale.w;
        uv_offset_scale.w *= -1.0f;
    }

    uint32_t order = custom_order.value >= 0 ? custom_order.value : m_order;

    m_draw_commands.emplace_back(batch_internal::DrawCommandNinePatch {
        .texture = ninepatch.texture(),
        .rotation = ninepatch.rotation(),
        .uv_offset_scale = uv_offset_scale,
        .color = ninepatch.color(),
        .margin = ninepatch.margin(),
        .position = ninepatch.position(),
        .size = ninepatch.size(),
        .offset = ninepatch.anchor().to_vec2(),
        .source_size = ninepatch.texture().size(),
        .output_size = ninepatch.size(),
        .order = order,
        .is_ui = is_ui,
    });

    if (custom_order.advance) m_order = ++order;

    ++m_ninepatch_count;
}

void Batch::DrawText(const RichTextSection* sections, size_t size, const glm::vec2& position, FontAsset key, bool is_ui, Order custom_order) {
    const Font& font = Assets::GetFont(key);

    float x = position.x;
    float y = position.y;

    uint32_t order = custom_order.value >= 0 ? custom_order.value : m_order;

    for (size_t i = 0; i < size; ++i) {
        const RichTextSection section = sections[i];
        const char* str = section.text.data();
        const size_t length = section.text.size();
        const float scale = section.size / font.font_size;

        for (size_t i = 0; i < length;) {
            const uint32_t c = next_utf8_codepoint(str, i);

            if (c == '\n') {
                y += section.size;
                x = position.x;
                continue;
            }

            const Glyph& ch = font.glyphs.find(c)->second;

            if (c == ' ') {
                x += (ch.advance >> 6) * scale;
                continue;
            }

            const float xpos = x + ch.bearing.x * scale; // ⌄ Make the origin at the top left corner
            const float ypos = y - ch.bearing.y * scale + section.size - font.ascender * scale;
            const glm::vec2 pos = glm::vec2(xpos, ypos);
            const glm::vec2 size = glm::vec2(ch.size) * scale;

            m_draw_commands.emplace_back(batch_internal::DrawCommandGlyph {
                .texture = font.texture,
                .color = section.color,
                .pos = pos,
                .size = size,
                .tex_size = ch.tex_size,
                .tex_uv = ch.texture_coords,
                .order = order,
                .is_ui = is_ui
            });

            x += (ch.advance >> 6) * scale;

            ++m_glyph_count;
        }
    }

    if (custom_order.advance) m_order = ++order;
}

void Batch::SortDrawCommands() {
    using namespace batch_internal;

    ZoneScopedN("Batch::SortDrawCommands");

    if (is_empty()) return;

    std::sort(
        m_draw_commands.begin(),
        m_draw_commands.end(),
        [](const DrawCommand& a, const DrawCommand& b) {
            const uint32_t a_order = a.order();
            const uint32_t b_order = b.order();

            if (a_order < b_order) return true;
            if (a_order > b_order) return false;

            return a.texture().id() < b.texture().id();
        }
    );

    Texture sprite_prev_texture;
    uint32_t sprite_count = 0;
    uint32_t sprite_total_count = 0;
    uint32_t sprite_vertex_offset = 0;
    uint32_t sprite_remaining = m_sprite_count;

    Texture glyph_prev_texture;
    uint32_t glyph_count = 0;
    uint32_t glyph_total_count = 0;
    uint32_t glyph_vertex_offset = 0;
    uint32_t glyph_remaining = m_glyph_count;

    Texture ninepatch_prev_texture;
    uint32_t ninepatch_count = 0;
    uint32_t ninepatch_total_count = 0;
    uint32_t ninepatch_vertex_offset = 0;
    uint32_t ninepatch_remaining = m_ninepatch_count;

    uint32_t prev_order = m_draw_commands[0].order();

    for (const DrawCommand& draw_command : m_draw_commands) {
        uint32_t new_order = prev_order;

        switch (draw_command.type()) {
        case DrawCommand::DrawSprite: {
            const DrawCommandSprite& sprite_data = draw_command.sprite_data();

            if (sprite_total_count == 0) {
                sprite_prev_texture = sprite_data.texture;
            }

            const uint32_t prev_texture_id = sprite_prev_texture.id();
            const uint32_t curr_texture_id = sprite_data.texture.id();

            if (sprite_count > 0 && prev_texture_id != curr_texture_id) {
                m_flush_queue.push_back(FlushData {
                    .texture = sprite_prev_texture,
                    .offset = sprite_vertex_offset,
                    .count = sprite_count,
                    .type = FlushDataType::Sprite
                });
                sprite_count = 0;
                sprite_vertex_offset = sprite_total_count;
            }

            int flags = 0;
            flags |= sprite_data.ignore_camera_zoom << SpriteFlags::IgnoreCameraZoom;
            flags |= sprite_data.is_ui << SpriteFlags::UI;

            m_sprite_buffer_ptr->position = sprite_data.position;
            m_sprite_buffer_ptr->rotation = sprite_data.rotation;
            m_sprite_buffer_ptr->size = sprite_data.size;
            m_sprite_buffer_ptr->offset = sprite_data.offset;
            m_sprite_buffer_ptr->uv_offset_scale = sprite_data.uv_offset_scale;
            m_sprite_buffer_ptr->color = sprite_data.color;
            m_sprite_buffer_ptr->outline_color = sprite_data.outline_color;
            m_sprite_buffer_ptr->outline_thickness = sprite_data.outline_thickness;
            m_sprite_buffer_ptr->flags = flags;
            m_sprite_buffer_ptr++;

            ++sprite_count;
            ++sprite_total_count;
            --sprite_remaining;

            if (sprite_remaining == 0) {
                m_flush_queue.push_back(FlushData {
                    .texture = sprite_data.texture,
                    .offset = sprite_vertex_offset,
                    .count = sprite_count,
                    .type = FlushDataType::Sprite
                });
                sprite_count = 0;
            }

            sprite_prev_texture = sprite_data.texture;
            new_order = sprite_data.order;
        } break;
        case DrawCommand::DrawGlyph: {
            const DrawCommandGlyph& glyph_data = draw_command.glyph_data();

            if (glyph_total_count == 0) {
                glyph_prev_texture = glyph_data.texture;
            }

            if (glyph_count > 0 && glyph_prev_texture.id() != glyph_data.texture.id()) {
                m_flush_queue.push_back(FlushData {
                    .texture = glyph_prev_texture,
                    .offset = glyph_vertex_offset,
                    .count = glyph_count,
                    .type = FlushDataType::Glyph
                });
                glyph_count = 0;
                glyph_vertex_offset = glyph_total_count;
            }

            m_glyph_buffer_ptr->color = glyph_data.color;
            m_glyph_buffer_ptr->pos = glyph_data.pos;
            m_glyph_buffer_ptr->size = glyph_data.size;
            m_glyph_buffer_ptr->tex_size = glyph_data.tex_size;
            m_glyph_buffer_ptr->uv = glyph_data.tex_uv;
            m_glyph_buffer_ptr->is_ui = glyph_data.is_ui;
            m_glyph_buffer_ptr++;

            ++glyph_count;
            ++glyph_total_count;
            --glyph_remaining;

            if (glyph_remaining == 0) {
                m_flush_queue.push_back(FlushData {
                    .texture = glyph_data.texture,
                    .offset = glyph_vertex_offset,
                    .count = glyph_count,
                    .type = FlushDataType::Glyph
                });
                glyph_count = 0;
            }

            glyph_prev_texture = glyph_data.texture;
            new_order = glyph_data.order;
        } break;
        case DrawCommand::DrawNinePatch: {
            const DrawCommandNinePatch& ninepatch_data = draw_command.ninepatch_data();

            if (ninepatch_total_count == 0) {
                ninepatch_prev_texture = ninepatch_data.texture;
            }

            const uint32_t prev_texture_id = ninepatch_prev_texture.id();
            const uint32_t curr_texture_id = ninepatch_data.texture.id();

            if (ninepatch_count > 0 && prev_texture_id != curr_texture_id) {
                m_flush_queue.push_back(FlushData {
                    .texture = ninepatch_prev_texture,
                    .offset = ninepatch_vertex_offset,
                    .count = ninepatch_count,
                    .type = FlushDataType::NinePatch
                });
                ninepatch_count = 0;
                ninepatch_vertex_offset = ninepatch_total_count;
            }

            int flags = ninepatch_data.is_ui << SpriteFlags::UI;

            m_ninepatch_buffer_ptr->position = ninepatch_data.position;
            m_ninepatch_buffer_ptr->rotation = ninepatch_data.rotation;
            m_ninepatch_buffer_ptr->margin = ninepatch_data.margin;
            m_ninepatch_buffer_ptr->size = ninepatch_data.size;
            m_ninepatch_buffer_ptr->offset = ninepatch_data.offset;
            m_ninepatch_buffer_ptr->source_size = ninepatch_data.source_size;
            m_ninepatch_buffer_ptr->output_size = ninepatch_data.output_size;
            m_ninepatch_buffer_ptr->uv_offset_scale = ninepatch_data.uv_offset_scale;
            m_ninepatch_buffer_ptr->color = ninepatch_data.color;
            m_ninepatch_buffer_ptr->flags = flags;
            m_ninepatch_buffer_ptr++;

            ++ninepatch_count;
            ++ninepatch_total_count;
            --ninepatch_remaining;

            if (ninepatch_remaining == 0) {
                m_flush_queue.push_back(FlushData {
                    .texture = ninepatch_data.texture,
                    .offset = ninepatch_vertex_offset,
                    .count = ninepatch_count,
                    .type = FlushDataType::NinePatch
                });
                ninepatch_count = 0;
            }

            ninepatch_prev_texture = ninepatch_data.texture;
            new_order = ninepatch_data.order;
        } break;
        }

        if (prev_order != new_order) {
            if (sprite_count > 1) {
                m_flush_queue.push_back(FlushData {
                    .texture = sprite_prev_texture,
                    .offset = sprite_vertex_offset,
                    .count = sprite_count,
                    .type = FlushDataType::Sprite
                });
                sprite_count = 0;
                sprite_vertex_offset = sprite_total_count;
            }

            if (glyph_count > 1) {
                m_flush_queue.push_back(FlushData {
                    .texture = glyph_prev_texture,
                    .offset = glyph_vertex_offset,
                    .count = glyph_count,
                    .type = FlushDataType::Glyph
                });
                glyph_count = 0;
                glyph_vertex_offset = glyph_total_count;
            }

            if (ninepatch_count > 1) {
                m_flush_queue.push_back(FlushData {
                    .texture = ninepatch_prev_texture,
                    .offset = ninepatch_vertex_offset,
                    .count = ninepatch_count,
                    .type = FlushDataType::NinePatch
                });
                ninepatch_count = 0;
                ninepatch_vertex_offset = ninepatch_total_count;
            }
        }

        prev_order = new_order;
    }
}