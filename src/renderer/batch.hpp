#ifndef TERRARIA_RENDER_BATCH_HPP
#define TERRARIA_RENDER_BATCH_HPP

#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <LLGL/LLGL.h>

#include "../math/rect.hpp"
#include "../types/sprite.hpp"
#include "../optional.hpp"

#include "assets.hpp"

constexpr size_t MAX_QUADS = 5000;
constexpr size_t MAX_VERTICES = MAX_QUADS * 4;
constexpr size_t MAX_INDICES = MAX_QUADS * 6;

struct GlyphVertex {
    glm::vec3 color;
    glm::vec2 pos;
    glm::vec2 uv;
    int is_ui;
};

struct SpriteData {
    glm::mat4 transform;
    glm::vec4 uv_offset_scale;
    glm::vec4 color;
    glm::vec4 outline_color;
    float outline_thickness;
    tl::optional<Texture> texture;
    int order;
    bool is_ui;
};

struct FlushData {
    tl::optional<Texture> texture;
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
    int order;
    bool is_ui;
};

class RenderBatch {
public:
    virtual void init() = 0;
    virtual void render() = 0;

    virtual void begin() = 0;
    virtual void terminate() = 0;

    inline void set_camera_frustum(const math::Rect& camera_frustum) { m_camera_frustum = camera_frustum; }
    inline void set_ui_frustum(const math::Rect& ui_frustum) { m_ui_frustum = ui_frustum; }

protected:
    math::Rect m_camera_frustum;
    math::Rect m_ui_frustum;
    LLGL::Buffer* m_vertex_buffer = nullptr;
    LLGL::PipelineState* m_pipeline = nullptr;
};

class RenderBatchSprite : public RenderBatch {
public:
    void draw_sprite(const BaseSprite& sprite, const glm::vec4& uv_offset_scale, const tl::optional<Texture>& sprite_texture, bool ui);
    void init() override;
    void render() override;
    void begin() override;
    void terminate() override;

    [[nodiscard]] inline bool is_empty() const { return m_sprites.empty(); }
private:
    void flush();
private:
    std::vector<SpriteData> m_sprites;
    std::vector<FlushData> m_sprite_flush_queue;

    SpriteVertex* m_buffer = nullptr;
    SpriteVertex* m_buffer_ptr = nullptr;
};

class RenderBatchGlyph : public RenderBatch {
public:
    void draw_glyph(const glm::vec2& pos, const glm::vec2& size, const glm::vec3& color, const Texture& font_texture, const glm::vec2& tex_uv, const glm::vec2& tex_size, bool ui);

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

    GlyphVertex* m_buffer = nullptr;
    GlyphVertex* m_buffer_ptr = nullptr;
    LLGL::Buffer* m_index_buffer = nullptr;
};

#endif
