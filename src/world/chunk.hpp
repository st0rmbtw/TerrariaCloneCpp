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

class RenderChunk {
public:
    RenderChunk(
        glm::uvec2 index,
        const glm::vec2& world_pos,
        const WorldData& world,
        ChunkInstance* block_data_arena,
        ChunkInstance* wall_data_arena
    ) : m_world_pos(world_pos * Constants::RENDER_CHUNK_SIZE),
        m_index(index)
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

    inline void set_blocks_dirty() noexcept {
        m_blocks_dirty = true;
    }

    inline void set_walls_dirty() noexcept {
        m_walls_dirty = true;
    }

    [[nodiscard]]
    inline bool dirty() const noexcept {
        return m_blocks_dirty || m_walls_dirty;
    }

    [[nodiscard]]
    inline uint16_t block_count() const noexcept {
        return m_block_count;
    }

    [[nodiscard]]
    inline uint16_t wall_count() const noexcept {
        return m_wall_count;
    }

    [[nodiscard]]
    inline LLGL::BufferArray* block_buffer_array() const noexcept {
        return m_block_buffer_array;
    }

    [[nodiscard]]
    inline LLGL::BufferArray* wall_buffer_array() const noexcept {
        return m_wall_buffer_array;
    }

    [[nodiscard]]
    inline LLGL::Buffer* block_instance_buffer() const noexcept {
        return m_block_instance_buffer;
    }

    [[nodiscard]]
    inline LLGL::Buffer* wall_instance_buffer() const noexcept {
        return m_wall_instance_buffer;
    }

private:
    glm::vec2 m_world_pos;
    glm::uvec2 m_index;
    LLGL::BufferArray* m_block_buffer_array = nullptr;
    LLGL::BufferArray* m_wall_buffer_array = nullptr;
    LLGL::Buffer* m_wall_instance_buffer = nullptr;
    LLGL::Buffer* m_block_instance_buffer = nullptr;
    uint16_t m_block_count = 0;
    uint16_t m_wall_count = 0;
    bool m_blocks_dirty = false;
    bool m_walls_dirty = false;
};

#endif