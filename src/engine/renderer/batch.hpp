#ifndef _RENDERER_BATCH_HPP_
#define _RENDERER_BATCH_HPP_

#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "../../types/texture.hpp"
#include "../../types/sprite.hpp"
#include "../../types/nine_patch.hpp"
#include "../../types/rich_text.hpp"
#include "../../types/order.hpp"
#include "../../assets.hpp"

#include "../../renderer/types.hpp"

#include "../../utils.hpp"

namespace batch_internal {
    static constexpr size_t MAX_QUADS = 2500;
    static constexpr size_t MAX_GLYPHS = 2500;

enum class FlushDataType : uint8_t {
    Sprite = 0,
    Glyph,
    NinePatch,
};

struct FlushData {
    Texture texture;
    uint32_t offset;
    uint32_t count;
    FlushDataType type;
};

struct DrawCommandSprite {
    Texture texture;
    glm::quat rotation;
    glm::vec4 uv_offset_scale;
    glm::vec4 color;
    glm::vec4 outline_color;
    glm::vec3 position;
    glm::vec2 size;
    glm::vec2 offset;
    uint32_t order;
    float outline_thickness;
    bool is_ui;
    bool ignore_camera_zoom;
    bool depth_enabled;
};

struct DrawCommandNinePatch {
    Texture texture;
    glm::quat rotation;
    glm::vec4 uv_offset_scale;
    glm::vec4 color;
    glm::uvec4 margin;
    glm::vec2 position;
    glm::vec2 size;
    glm::vec2 offset;
    glm::vec2 source_size;
    glm::vec2 output_size;
    uint32_t order;
    bool is_ui;
};

struct DrawCommandGlyph {
    Texture texture;
    glm::vec3 color;
    glm::vec2 pos;
    glm::vec2 size;
    glm::vec2 tex_size;
    glm::vec2 tex_uv;
    uint32_t order;
    bool is_ui;
};

class DrawCommand {
public:
    enum Type : uint8_t {
        DrawSprite = 0,
        DrawGlyph,
        DrawNinePatch
    };

    DrawCommand(DrawCommandSprite sprite_data) :
        m_sprite_data(sprite_data),
        m_type(Type::DrawSprite) {}

    DrawCommand(DrawCommandGlyph glyph_data) :
        m_glyph_data(glyph_data),
        m_type(Type::DrawGlyph) {}

    DrawCommand(DrawCommandNinePatch ninepatch_data) :
        m_ninepatch_data(ninepatch_data),
        m_type(Type::DrawNinePatch) {}

    [[nodiscard]] inline Type type() const { return m_type; }

    [[nodiscard]] inline uint32_t order() const {
        switch (m_type) {
        case DrawSprite: return m_sprite_data.order;
        case DrawGlyph: return m_glyph_data.order;
        case DrawNinePatch: return m_ninepatch_data.order;
        }
    }

    [[nodiscard]] inline const Texture& texture() const {
        switch (m_type) {
        case DrawSprite: return m_sprite_data.texture;
        case DrawGlyph: return m_glyph_data.texture;
        case DrawNinePatch: return m_ninepatch_data.texture;
        }
    }

    [[nodiscard]] inline const DrawCommandSprite& sprite_data() const { return m_sprite_data; }
    [[nodiscard]] inline const DrawCommandGlyph& glyph_data() const { return m_glyph_data; }
    [[nodiscard]] inline const DrawCommandNinePatch& ninepatch_data() const { return m_ninepatch_data; }

private:
    union {
        DrawCommandNinePatch m_ninepatch_data;
        DrawCommandSprite m_sprite_data;
        DrawCommandGlyph m_glyph_data;
    };

    Type m_type;
};

};

class Batch {
    friend class Renderer;

public:
    using DrawCommands = std::vector<batch_internal::DrawCommand>;
    using FlushQueue = std::vector<batch_internal::FlushData>;

    Batch() {
        m_draw_commands.reserve(100);
        m_sprite_buffer = checked_alloc<SpriteInstance>(batch_internal::MAX_QUADS);
        m_sprite_buffer_ptr = m_sprite_buffer;

        m_glyph_buffer = checked_alloc<GlyphInstance>(batch_internal::MAX_GLYPHS);
        m_glyph_buffer_ptr = m_glyph_buffer;

        m_ninepatch_buffer = checked_alloc<NinePatchInstance>(batch_internal::MAX_QUADS);
        m_ninepatch_buffer_ptr = m_ninepatch_buffer;
    };

    inline void BeginOrderMode() { m_order_mode = true; }
    inline void EndOrderMode() { m_order_mode = true; }

    void DrawText(const RichTextSection* sections, size_t size, const glm::vec2& position, FontAsset font, bool is_ui, Order order = -1);
    void DrawSprite(const Sprite& sprite, bool is_ui, Order order = -1);
    void DrawAtlasSprite(const TextureAtlasSprite& sprite, bool is_ui, Order order = -1);
    void DrawNinePatch(const NinePatch& ninepatch, bool is_ui, Order order = -1);

    inline void DrawSprite(const Sprite& sprite, Order order = -1) { DrawSprite(sprite, false, order); }
    inline void DrawSpriteUI(const Sprite& sprite, Order order = -1) { DrawSprite(sprite, true, order); }

    inline void DrawAtlasSprite(const TextureAtlasSprite& sprite, Order order = -1) { DrawAtlasSprite(sprite, false, order); }
    inline void DrawAtlasSpriteUI(const TextureAtlasSprite& sprite, Order order = -1) { DrawAtlasSprite(sprite, true, order); }

    inline void DrawNinePatch(const NinePatch& ninepatch, Order order = -1) { DrawNinePatch(ninepatch, false, order); }
    inline void DrawNinePatchUI(const NinePatch& ninepatch, Order order = -1) { DrawNinePatch(ninepatch, true, order); }

    inline void DrawText(const RichTextSection* sections, size_t size, const glm::vec2& position, FontAsset font, Order order = -1) {
        DrawText(sections, size, position, font, false, order);
    }

    inline void DrawTextUI(const RichTextSection* sections, size_t size, const glm::vec2& position, FontAsset font, Order order = -1) {
        DrawText(sections, size, position, font, true, order);
    }

    inline void Reset() {
        m_draw_commands.clear();
        m_flush_queue.clear();
        m_order = 0;

        m_sprite_buffer_ptr = m_sprite_buffer;
        m_sprite_count = 0;

        m_glyph_buffer_ptr = m_glyph_buffer;
        m_glyph_count = 0;

        m_ninepatch_buffer_ptr = m_ninepatch_buffer;
        m_ninepatch_count = 0;
    }

    [[nodiscard]]
    inline bool is_empty() const { return m_draw_commands.empty(); }

    [[nodiscard]]
    inline uint32_t order() const { return m_order; }

private:
    void SortDrawCommands();

    void AddSpriteDrawCommand(const BaseSprite& sprite, const glm::vec4& uv_offset_scale, const Texture& texture, uint32_t order, bool is_ui) {
        m_draw_commands.emplace_back(batch_internal::DrawCommandSprite {
            .texture = texture,
            .rotation = sprite.rotation(),
            .uv_offset_scale = uv_offset_scale,
            .color = sprite.color(),
            .outline_color = sprite.outline_color(),
            .position = glm::vec3(sprite.position(), sprite.z()),
            .size = sprite.size(),
            .offset = sprite.anchor().to_vec2(),
            .order = order,
            .outline_thickness = sprite.outline_thickness(),
            .is_ui = is_ui,
            .ignore_camera_zoom = sprite.ignore_camera_zoom(),
            .depth_enabled = false
        });
    }

    [[nodiscard]] inline FlushQueue& flush_queue() { return m_flush_queue; }

    [[nodiscard]] inline SpriteInstance* sprite_buffer() { return m_sprite_buffer; }
    [[nodiscard]] inline GlyphInstance* glyph_buffer() { return m_glyph_buffer; }
    [[nodiscard]] inline NinePatchInstance* ninepatch_buffer() { return m_ninepatch_buffer; }

    [[nodiscard]] inline size_t sprite_buffer_size() {
        return (size_t) m_sprite_buffer_ptr - (size_t) m_sprite_buffer;
    }

    [[nodiscard]] inline size_t glyph_buffer_size() {
        return (size_t) m_glyph_buffer_ptr - (size_t) m_glyph_buffer;
    }

    [[nodiscard]] inline size_t ninepatch_buffer_size() {
        return (size_t) m_ninepatch_buffer_ptr - (size_t) m_ninepatch_buffer;
    }

private:
    DrawCommands m_draw_commands;
    FlushQueue m_flush_queue;

    SpriteInstance* m_sprite_buffer = nullptr;
    SpriteInstance* m_sprite_buffer_ptr = nullptr;

    GlyphInstance* m_glyph_buffer = nullptr;
    GlyphInstance* m_glyph_buffer_ptr = nullptr;

    NinePatchInstance* m_ninepatch_buffer = nullptr;
    NinePatchInstance* m_ninepatch_buffer_ptr = nullptr;

    uint32_t m_order = 0;

    uint32_t m_sprite_count = 0;
    uint32_t m_glyph_count = 0;
    uint32_t m_ninepatch_count = 0;

    bool m_order_mode = false;
};

#endif