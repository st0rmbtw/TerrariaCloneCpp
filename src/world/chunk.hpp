#ifndef WORLD_CHUNK_HPP
#define WORLD_CHUNK_HPP

#pragma once

#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <LLGL/Buffer.h>
#include <LLGL/BufferArray.h>
#include <LLGL/Texture.h>

#include "../constants.hpp"

#include "world_data.hpp"

struct RenderChunk {
    glm::vec2 world_pos;
    glm::uvec2 index;
    LLGL::BufferArray* block_buffer_array = nullptr;
    LLGL::BufferArray* wall_buffer_array = nullptr;
    LLGL::Buffer* wall_instance_buffer = nullptr;
    LLGL::Buffer* block_instance_buffer = nullptr;
    uint16_t blocks_count = 0;
    uint16_t walls_count = 0;
    bool blocks_dirty = true;
    bool walls_dirty = true;

    RenderChunk(
        glm::uvec2 index,
        const glm::vec2& world_pos,
        const WorldData& world
    ) : world_pos(world_pos.x * Constants::RENDER_CHUNK_SIZE, world_pos.y * Constants::RENDER_CHUNK_SIZE),
        index(index)
    {
        build_mesh(world);
    }

    void build_mesh(const WorldData& world);
    void destroy();

    [[nodiscard]] inline bool dirty() const { return blocks_dirty || walls_dirty; }

    [[nodiscard]] inline bool blocks_empty() const { return blocks_count == 0; }
    [[nodiscard]] inline bool walls_empty() const { return walls_count == 0; }
};

#endif