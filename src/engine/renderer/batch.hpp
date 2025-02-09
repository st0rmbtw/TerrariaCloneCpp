#ifndef _ENGINE_RENDERER_BATCH_HPP_
#define _ENGINE_RENDERER_BATCH_HPP_

#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "../types/texture.hpp"
#include "../types/sprite.hpp"
#include "../types/nine_patch.hpp"
#include "../types/rich_text.hpp"
#include "../types/order.hpp"

#include "../../assets.hpp"

namespace batch_internal {

enum class FlushDataType : uint8_t {
    Sprite = 0,
    Glyph,
    NinePatch,
};

struct FlushData {
    Texture texture;
    uint32_t offset;
    uint32_t count;
    uint32_t order;
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
};

struct DrawCommandGlyph {
    Texture texture;
    glm::vec3 color;
    glm::vec2 pos;
    glm::vec2 size;
    glm::vec2 tex_size;
    glm::vec2 tex_uv;
    uint32_t order;
};

class DrawCommand {
public:
    enum Type : uint8_t {
        DrawSprite = 0,
        DrawGlyph,
        DrawNinePatch
    };

    DrawCommand(DrawCommandSprite sprite_data, uint32_t id) :
        m_sprite_data(sprite_data),
        m_id(id),
        m_type(Type::DrawSprite) {}

    DrawCommand(DrawCommandGlyph glyph_data, uint32_t id) :
        m_glyph_data(glyph_data),
        m_id(id),
        m_type(Type::DrawGlyph) {}

    DrawCommand(DrawCommandNinePatch ninepatch_data, uint32_t id) :
        m_ninepatch_data(ninepatch_data),
        m_id(id),
        m_type(Type::DrawNinePatch) {}

    [[nodiscard]] inline Type type() const { return m_type; }
    [[nodiscard]] inline uint32_t id() const { return m_id; }

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

    uint32_t m_id;

    Type m_type;
};

inline static glm::vec4 get_uv_offset_scale(bool flip_x, bool flip_y) {
    glm::vec4 uv_offset_scale = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);

    if (flip_x) {
        uv_offset_scale.x += uv_offset_scale.z;
        uv_offset_scale.z *= -1.0f;
    }

    if (flip_y) {
        uv_offset_scale.y += uv_offset_scale.w;
        uv_offset_scale.w *= -1.0f;
    }

    return uv_offset_scale;
}

};

class Batch {
    friend class Renderer;

public:
    using DrawCommands = std::vector<batch_internal::DrawCommand>;
    using FlushQueue = std::vector<batch_internal::FlushData>;

    Batch() {
        m_draw_commands.reserve(500);
        m_flush_queue.reserve(100);
    };

    Batch(const Batch&) = delete;
    Batch& operator=(const Batch&) = delete;

    inline void set_depth_enabled(bool depth_enabled) { m_depth_enabled = depth_enabled; }
    inline void set_is_ui(bool is_ui) { m_is_ui = is_ui; }

    inline void BeginOrderMode(int order, bool advance) {
        m_order_mode = true;
        m_global_order.value = order < 0 ? m_order : order;
        m_global_order.advance = advance;
    }

    inline void BeginOrderMode(int order = -1) {
        BeginOrderMode(order, true);
    }

    inline void BeginOrderMode(bool advance) {
        BeginOrderMode(-1, advance);
    }

    inline void EndOrderMode() {
        m_order_mode = false;
        m_global_order.value = 0;
        m_global_order.advance = false;
    }

    uint32_t DrawText(const RichTextSection* sections, size_t size, const glm::vec2& position, FontAsset font, Order order = -1);

    uint32_t DrawAtlasSprite(const TextureAtlasSprite& sprite, Order order = -1);

    inline uint32_t DrawSprite(const Sprite& sprite, Order custom_order = -1) {
        const glm::vec4 uv_offset_scale = batch_internal::get_uv_offset_scale(sprite.flip_x(), sprite.flip_y());
        return AddSpriteDrawCommand(sprite, uv_offset_scale, sprite.texture(), custom_order);
    }

    inline uint32_t DrawNinePatch(const NinePatch& ninepatch, Order custom_order = -1) {
        const glm::vec4 uv_offset_scale = batch_internal::get_uv_offset_scale(ninepatch.flip_x(), ninepatch.flip_y());
        return AddNinePatchDrawCommand(ninepatch, uv_offset_scale, custom_order);
    }

    inline void Reset() {
        m_draw_commands.clear();
        m_flush_queue.clear();
        m_order = 0;

        m_sprite_instance_offset = 0;
        m_sprite_count = 0;

        m_glyph_instance_offset = 0;
        m_glyph_count = 0;

        m_ninepatch_instance_offset = 0;
        m_ninepatch_count = 0;

        m_draw_commands_done = 0;
    }

    [[nodiscard]]
    inline bool depth_enabled() const { return m_depth_enabled; }

    [[nodiscard]]
    inline bool is_ui() const { return m_is_ui; }

    [[nodiscard]]
    inline uint32_t order() const { return m_order; }

    [[nodiscard]]
    inline size_t sprite_count() const { return m_sprite_count; }

    [[nodiscard]]
    inline size_t glyph_count() const { return m_glyph_count; }

    [[nodiscard]]
    inline size_t ninepatch_count() const { return m_ninepatch_count; }

private:
    uint32_t AddSpriteDrawCommand(const BaseSprite& sprite, const glm::vec4& uv_offset_scale, const Texture& texture, Order custom_order);
    uint32_t AddNinePatchDrawCommand(const NinePatch& ninepatch, const glm::vec4& uv_offset_scale, Order custom_order);
    void AddGlyphDrawCommand(const batch_internal::DrawCommandGlyph& command);

    inline void set_sprite_offset(size_t offset) { m_sprite_instance_offset = offset; }
    inline void set_glyph_offset(size_t offset) { m_glyph_instance_offset = offset; }
    inline void set_ninepatch_offset(size_t offset) { m_ninepatch_instance_offset = offset; }

    inline void set_sprite_count(size_t count) { m_sprite_count = count; }
    inline void set_glyph_count(size_t count) { m_glyph_count = count; }
    inline void set_ninepatch_count(size_t count) { m_ninepatch_count = count; }

    inline void set_draw_commands_done(size_t count) { m_draw_commands_done = count; }

    [[nodiscard]] inline size_t sprite_offset() const { return m_sprite_instance_offset; }
    [[nodiscard]] inline size_t glyph_offset() const { return m_glyph_instance_offset; }
    [[nodiscard]] inline size_t ninepatch_offset() const { return m_ninepatch_instance_offset; }

    [[nodiscard]] inline size_t draw_commands_done() const { return m_draw_commands_done; }

    [[nodiscard]] inline const FlushQueue& flush_queue() const { return m_flush_queue; }
    [[nodiscard]] inline FlushQueue& flush_queue() { return m_flush_queue; }

    [[nodiscard]] inline const DrawCommands& draw_commands() const { return m_draw_commands; }
    [[nodiscard]] inline DrawCommands& draw_commands() { return m_draw_commands; }

private:
    static constexpr size_t MAX_QUADS = 2500;
    static constexpr size_t MAX_GLYPHS = 2500;

    DrawCommands m_draw_commands;
    FlushQueue m_flush_queue;

    size_t m_sprite_instance_offset = 0;
    size_t m_glyph_instance_offset = 0;
    size_t m_ninepatch_instance_offset = 0;

    uint32_t m_order = 0;

    uint32_t m_sprite_count = 0;
    uint32_t m_glyph_count = 0;
    uint32_t m_ninepatch_count = 0;

    uint32_t m_draw_commands_done = 0;

    Order m_global_order;

    bool m_order_mode = false;
    bool m_depth_enabled = false;
    bool m_is_ui = false;
};

#endif