#ifndef TERRARIA_RENDERER_TYPES
#define TERRARIA_RENDERER_TYPES

#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

struct SpriteVertex {
    float x;
    float y;
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
    int has_texture;
    int is_ui;
    int is_nonscalable;
    int is_world;
};

struct GlyphVertex {
    float x;
    float y;
};

struct GlyphInstance {
    glm::vec3 color;
    glm::vec3 pos;
    glm::vec2 size;
    glm::vec2 tex_size;
    glm::vec2 uv;
    int is_ui;
};

struct ParticleVertex {
    float x;
    float y;
    glm::vec2 inv_tex_size;
    glm::vec2 tex_size;
};

struct ParticleInstance {
    glm::vec2 uv;
    float depth;
};

struct ChunkInstance {
    glm::vec2 position;
    glm::vec2 atlas_pos;
    glm::vec2 world_pos;
    uint32_t tile_id;
    uint32_t tile_type;
};

struct ChunkVertex {
    float x, y;
    glm::vec2 wall_tex_size;
    glm::vec2 tile_tex_size;
    glm::vec2 wall_padding;
    glm::vec2 tile_padding;
};

#endif