#ifndef RENDERER_TYPES_HPP
#define RENDERER_TYPES_HPP

#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

struct Vertex {
    float x;
    float y;

    explicit Vertex(float x, float y) : x(x), y(y) {}
};

struct SpriteInstance {
    glm::vec2 position;
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

struct ParticleVertex {
    float x;
    float y;
    glm::vec2 inv_tex_size;
    glm::vec2 tex_size;

    explicit ParticleVertex(float x, float y, glm::vec2 inv_tex_size, glm::vec2 tex_size) :
        x(x), y(y), inv_tex_size(inv_tex_size), tex_size(tex_size) {}
};

struct ParticleInstance {
    glm::vec2 uv;
    float depth;
    uint32_t id;
    uint32_t is_world;
};

struct ChunkInstance {
    glm::vec2 position;
    glm::vec2 atlas_pos;
    glm::vec2 world_pos;
    uint16_t tile_data;

    ChunkInstance(glm::vec2 position, glm::vec2 atlas_pos, glm::vec2 world_pos, uint16_t tile_data) :
        position(position),
        atlas_pos(atlas_pos),
        world_pos(world_pos),
        tile_data(tile_data) {}
};

struct ChunkVertex {
    float x, y;
    glm::vec2 wall_tex_size;
    glm::vec2 tile_tex_size;
    glm::vec2 wall_padding;
    glm::vec2 tile_padding;

    explicit ChunkVertex(float x, float y, glm::vec2 wall_tex_size, glm::vec2 tile_tex_size, glm::vec2 wall_padding, glm::vec2 tile_padding) :
        x(x), y(y),
        wall_tex_size(wall_tex_size), tile_tex_size(tile_tex_size), 
        wall_padding(wall_padding), tile_padding(tile_padding) {}
};

struct BackgroundVertex {
    explicit BackgroundVertex(glm::vec2 position, glm::vec2 texture_size) :
        position(position),
        texture_size(texture_size) {}

    glm::vec2 position;
    glm::vec2 texture_size;
};

struct BackgroundInstance {
    glm::vec2 position;
    glm::vec2 size;
    glm::vec2 tex_size;
    glm::vec2 speed;
    uint32_t id;
    int flags;
};

#endif