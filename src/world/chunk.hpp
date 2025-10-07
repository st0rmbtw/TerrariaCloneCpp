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
#include "lightmap.hpp"

struct LightMapChunkNeighbors {
    const LightMap* top = nullptr;
    const LightMap* bottom = nullptr;
    const LightMap* left = nullptr;
    const LightMap* right = nullptr;
};

LightMap build_lightmap_chunk(glm::uvec2 index, const WorldData& world, LightMapChunkNeighbors neighbors);

struct StaticLightMapChunk {
    LightMap lightmap;
    glm::uvec2 index;

    LLGL::Texture* texture = nullptr;
    LLGL::Buffer* vertex_buffer = nullptr;

    StaticLightMapChunk() = default;
    StaticLightMapChunk(glm::uvec2 index, LightMap lightmap);

    StaticLightMapChunk(StaticLightMapChunk&& other) noexcept {
        operator=(std::move(other));
    }

    void copy_lightmap_area(const Color* from_colors, const LightMask* from_mask, glm::uvec2 from_offset, uint32_t from_stride, glm::uvec2 to_offset, glm::uvec2 size);

    void blur_from_top(const LightMap& top);
    void blur_from_bottom(const LightMap& bottom);
    void blur_from_left(const LightMap& left);
    void blur_from_right(const LightMap& right);

    StaticLightMapChunk& operator =(StaticLightMapChunk&& other) noexcept {
        index = other.index;
        lightmap = std::move(other.lightmap);
        texture = other.texture;
        vertex_buffer = other.vertex_buffer;

        other.texture = nullptr;
        other.vertex_buffer = nullptr;

        return *this;
    }

    ~StaticLightMapChunk();
};

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

    RenderChunk(RenderChunk&& other) noexcept {
        operator=(std::move(other));
    }

    RenderChunk& operator=(RenderChunk&& other) noexcept {
        m_block_instance_buffer = other.m_block_instance_buffer;
        m_wall_instance_buffer = other.m_wall_instance_buffer;
        m_block_buffer_array = other.m_block_buffer_array;
        m_wall_buffer_array = other.m_wall_buffer_array;

        other.m_block_instance_buffer = nullptr;
        other.m_wall_instance_buffer = nullptr;
        other.m_block_buffer_array = nullptr;
        other.m_wall_buffer_array = nullptr;
        
        m_world_pos = other.m_world_pos;
        m_index = other.m_index;
        m_block_count = other.m_block_count;
        m_wall_count = other.m_wall_count;
        m_blocks_dirty = other.m_blocks_dirty;
        m_walls_dirty = other.m_walls_dirty;

        return *this;
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

    ~RenderChunk() {
        destroy();
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