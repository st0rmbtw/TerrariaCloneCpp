#ifndef TERRARIA_RENDER_BATCH_HPP
#define TERRARIA_RENDER_BATCH_HPP

#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <LLGL/LLGL.h>

#include "../types/sprite.hpp"
#include "../optional.hpp"

constexpr size_t MAX_QUADS = 5000;
constexpr size_t MAX_VERTICES = MAX_QUADS * 4;
constexpr size_t MAX_INDICES = MAX_QUADS * 6;

struct SpriteVertex {
    glm::vec3 position;
    glm::quat rotation;
    glm::vec2 size;
    glm::vec2 offset;
    glm::vec4 uv_offset_scale;
    glm::vec4 color;
    glm::vec4 outline_color;
    float outline_thickness;
    int has_texture;
    int is_ui;
    int is_nonscalable;
};

struct GlyphVertex {
    glm::vec3 color;
    glm::vec3 pos;
    glm::vec2 uv;
    int is_ui;
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
    bool is_nonscalable;
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
    LLGL::PipelineState* m_pipeline = nullptr;
};

class RenderBatchSprite : public RenderBatch {
public:
    void draw_sprite(const BaseSprite& sprite, const glm::vec4& uv_offset_scale, const tl::optional<Texture>& sprite_texture, bool ui, int depth);
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

    GlyphVertex* m_buffer = nullptr;
    GlyphVertex* m_buffer_ptr = nullptr;
    LLGL::Buffer* m_index_buffer = nullptr;
};

#endif
