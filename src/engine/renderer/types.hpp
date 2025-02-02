#ifndef _ENGINE_RENDERER_TYPES_HPP_
#define _ENGINE_RENDERER_TYPES_HPP_

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct Vertex {
    float x;
    float y;

    explicit Vertex(float x, float y) : x(x), y(y) {}
};

struct SpriteInstance {
    glm::vec3 position;
    glm::quat rotation;
    glm::vec2 size;
    glm::vec2 offset;
    glm::vec4 uv_offset_scale;
    glm::vec4 color;
    glm::vec4 outline_color;
    float outline_thickness;
    int flags;
};

struct NinePatchInstance {
    glm::quat rotation;
    glm::vec2 position;
    glm::vec2 size;
    glm::vec2 offset;
    glm::vec2 source_size;
    glm::vec2 output_size;
    glm::uvec4 margin;
    glm::vec4 uv_offset_scale;
    glm::vec4 color;
    int flags;
};

struct GlyphInstance {
    glm::vec3 color;
    glm::vec2 pos;
    glm::vec2 size;
    glm::vec2 tex_size;
    glm::vec2 uv;
    int is_ui;
};

#endif