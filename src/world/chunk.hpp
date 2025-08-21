#pragma once

#ifndef WORLD_CHUNK_HPP_
#define WORLD_CHUNK_HPP_

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <LLGL/Buffer.h>
#include <LLGL/BufferArray.h>
#include <LLGL/Texture.h>

#include "../constants.hpp"
#include "../renderer/types.hpp"

#include "world_data.hpp"

struct RenderChunk {
    glm::vec2 world_pos;
    glm::uvec2 index;
    LLGL::BufferArray* block_buffer_array = nullptr;
    LLGL::BufferArray* wall_buffer_array = nullptr;
    LLGL::Buffer* wall_instance_buffer = nullptr;
    LLGL::Buffer* block_instance_buffer = nullptr;
    uint16_t block_count = 0;
    uint16_t wall_count = 0;
    bool blocks_dirty = false;
    bool walls_dirty = false;

    RenderChunk(
        glm::uvec2 index,
        const glm::vec2& world_pos,
        const WorldData& world,
        ChunkInstance* block_data_arena,
        ChunkInstance* wall_data_arena
    ) : world_pos(world_pos * Constants::RENDER_CHUNK_SIZE),
        index(index)
    {
        build_mesh(world, block_data_arena, wall_data_arena);
    }

    void build_mesh(
        const WorldData& world,
        ChunkInstance* block_data_arena,
        ChunkInstance* wall_data_arena
    );

    void rebuild_mesh(
        const WorldData& world,
        ChunkInstance* block_data_arena,
        ChunkInstance* wall_data_arena
    );

    void destroy();

    [[nodiscard]]
    inline bool dirty() const noexcept {
        return blocks_dirty || walls_dirty;
    }
};

#endif