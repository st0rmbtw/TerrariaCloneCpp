#ifndef TERRARIA_RENDER_BATCH_HPP
#define TERRARIA_RENDER_BATCH_HPP

#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <LLGL/LLGL.h>

#include "renderer/assets.hpp"
#include "math/rect.hpp"
#include "types/sprite.hpp"
#include "optional.hpp"

constexpr size_t MAX_QUADS = 5000;
constexpr size_t MAX_VERTICES = MAX_QUADS * 4;
constexpr size_t MAX_INDICES = MAX_QUADS * 6;

struct GlyphVertex {
    glm::vec4 transform_col_0;
    glm::vec4 transform_col_1;
    glm::vec4 transform_col_2;
    glm::vec4 transform_col_3;
    glm::vec3 color;
    glm::vec2 size;
    glm::vec2 uv;
    float is_ui;
};

struct SpriteData {
    glm::mat4 transform;
    glm::vec4 uv_offset_scale;
    glm::vec4 color;
    tl::optional<Texture> texture;
    int order;
    bool is_ui;
};

struct SpriteFlush {
    tl::optional<Texture> texture;
    int vertex_offset;
    uint32_t index_count;
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
    LLGL::Buffer* m_index_buffer = nullptr;
    LLGL::PipelineState* m_pipeline = nullptr;
};

class RenderBatchSprite : public RenderBatch {
public:
    void draw_sprite(const BaseSprite& sprite, const glm::vec4& uv_offset_scale, const tl::optional<Texture>& sprite_texture, bool ui);
    void init() override;
    void render() override;
    void begin() override;
    void terminate() override;

    inline void clear_sprites() { m_sprites.clear(); }

    [[nodiscard]] inline bool is_empty() const { return m_sprites.empty(); }
private:
    void flush();
private:
    std::vector<SpriteData> m_sprites;
    std::vector<SpriteFlush> m_sprite_flush_queue;

    SpriteVertex* m_buffer = nullptr;
    SpriteVertex* m_buffer_ptr = nullptr;
};

class RenderBatchGlyph : public RenderBatch {
public:
    void draw_glyph(const glm::mat4& transform, const glm::vec3& color, /* const Texture& font_texture, */ const glm::vec2& uv, const glm::vec2& size, bool ui);

    void init() override;
    void render() override;
    void begin() override;
    void terminate() override;
private:
    GlyphVertex* m_buffer = nullptr;
    GlyphVertex* m_buffer_ptr = nullptr;
};

#endif
