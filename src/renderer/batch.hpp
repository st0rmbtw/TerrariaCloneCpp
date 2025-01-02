#ifndef TERRARIA_RENDER_BATCH_HPP
#define TERRARIA_RENDER_BATCH_HPP

#pragma once

#include <vector>
#include <optional>

#include <glm/glm.hpp>
#include <LLGL/LLGL.h>

#include "types.hpp"

#include "../types/sprite.hpp"
#include "../types/depth.hpp"

constexpr size_t MAX_QUADS = 5000 / 2;

namespace SpriteFlags {
    enum : uint8_t {
        HasTexture = 0,
        UI,
        World,
        IgnoreCameraZoom,
    };
};

struct SpriteData {
    glm::vec2 position;
    glm::quat rotation;
    glm::vec2 size;
    glm::vec2 offset;
    glm::vec4 uv_offset_scale;
    glm::vec4 color;
    glm::vec4 outline_color;
    float outline_thickness;
    Texture texture;
    uint32_t order;
    bool is_ui;
    bool ignore_camera_zoom;
};

struct FlushData {
    Texture texture;
    int offset;
    uint32_t count;
};

struct GlyphData {
    Texture texture;
    glm::vec3 color;
    glm::vec2 pos;
    glm::vec2 size;
    glm::vec2 tex_size;
    glm::vec2 tex_uv;
    uint32_t order;
    bool is_ui;
};

class RenderBatch {
public:
    virtual void init() = 0;
    virtual void render() = 0;

    virtual void begin() = 0;
    virtual void terminate() = 0;

protected:
    LLGL::Buffer* m_vertex_buffer = nullptr;
    LLGL::Buffer* m_instance_buffer = nullptr;
    LLGL::BufferArray* m_buffer_array = nullptr;

    LLGL::PipelineState* m_pipeline = nullptr;
};

class RenderBatchSprite : public RenderBatch {
public:
    void draw_sprite(const BaseSprite& sprite, const glm::vec4& uv_offset_scale, const std::optional<Texture>& sprite_texture, bool is_ui, Depth depth);
    void draw_world_sprite(const BaseSprite& sprite, const glm::vec4& uv_offset_scale, const std::optional<Texture>& sprite_texture, Depth depth);
    void init() override;
    void render() override;
    void render_world();
    void begin() override;
    void terminate() override;
private:
    void flush();
    void flush_world();
private:
    std::vector<SpriteData> m_sprites;
    std::vector<FlushData> m_sprite_flush_queue;

    std::vector<SpriteData> m_world_sprites;
    std::vector<FlushData> m_world_sprite_flush_queue;

    LLGL::Buffer* m_world_instance_buffer = nullptr;
    LLGL::BufferArray* m_world_buffer_array = nullptr;

    SpriteInstance* m_buffer = nullptr;
    SpriteInstance* m_buffer_ptr = nullptr;

    SpriteInstance* m_world_buffer = nullptr;
    SpriteInstance* m_world_buffer_ptr = nullptr;
};

class RenderBatchGlyph : public RenderBatch {
public:
    void draw_glyph(const glm::vec2& pos, const glm::vec2& size, const glm::vec3& color, const Texture& font_texture, const glm::vec2& tex_uv, const glm::vec2& tex_size, bool ui, uint32_t depth);

    void init() override;
    void render() override;
    void begin() override;
    void terminate() override;

    [[nodiscard]] inline bool is_empty() const { return m_glyphs.empty(); }
private:
    void flush();
private:
    std::vector<GlyphData> m_glyphs;
    std::vector<FlushData> m_glyphs_flush_queue;

    GlyphInstance* m_buffer = nullptr;
    GlyphInstance* m_buffer_ptr = nullptr;
};

#endif
