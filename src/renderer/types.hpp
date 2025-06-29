#ifndef RENDERER_TYPES_HPP
#define RENDERER_TYPES_HPP

#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

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
    glm::vec2 atlas_pos;
    glm::vec2 world_pos;
    uint16_t position;
    uint16_t tile_data;

    ChunkInstance(uint16_t position, glm::vec2 atlas_pos, glm::vec2 world_pos, uint16_t tile_data) :
        atlas_pos(atlas_pos),
        world_pos(world_pos),
        position(position),
        tile_data(tile_data) {}
};

struct BackgroundVertex {
    explicit BackgroundVertex(glm::vec2 position, glm::vec2 texture_size) :
        position(position),
        texture_size(texture_size) {}

    glm::vec2 position;
    glm::vec2 texture_size;
};

struct StaticLightMapChunkVertex {
    explicit StaticLightMapChunkVertex(glm::vec2 position, glm::vec2 uv) :
        position(position),
        uv(uv) {}

    glm::vec2 position;
    glm::vec2 uv;
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