#ifndef WORLD_CHUNK_HPP
#define WORLD_CHUNK_HPP

#pragma once

#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <LLGL/Buffer.h>
#include "../constants.hpp"

using Constants::RENDER_CHUNK_SIZE;
using Constants::RENDER_CHUNK_SIZE_U;

struct RenderChunk {
    glm::vec2 world_pos;
    glm::uvec2 index;
    LLGL::Buffer* block_vertex_buffer = nullptr;
    LLGL::Buffer* wall_vertex_buffer = nullptr;
    uint16_t blocks_count = 0;
    uint16_t walls_count = 0;
    bool blocks_dirty = true;
    bool walls_dirty = true;

    RenderChunk(
        glm::uvec2 index,
        const glm::vec2& world_pos,
        const class World& world
    ) : world_pos(world_pos.x * RENDER_CHUNK_SIZE, world_pos.y * RENDER_CHUNK_SIZE),
        index(std::move(index))
    {
        build_mesh(world);
    }

    void build_mesh(const class World& world);
    void destroy();

    [[nodiscard]] inline bool dirty() const { return blocks_dirty || walls_dirty; }

    [[nodiscard]] inline bool blocks_empty() const { return blocks_count == 0; }
    [[nodiscard]] inline bool walls_empty() const { return walls_count == 0; }
};

#endif