#pragma once

#ifndef WORLD_CHUNK_HPP_
#define WORLD_CHUNK_HPP_

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <LLGL/Buffer.h>
#include <LLGL/BufferArray.h>
#include <LLGL/Texture.h>

#include <SGE/utils/llgl.hpp>
#include <SGE/renderer/renderer.hpp>

#include "../constants.hpp"
#include "../renderer/types.hpp"

#include "world_data.hpp"
#include "lightmap.hpp"

struct StaticLightMapChunk {
    glm::uvec2 index;

    std::shared_ptr<sge::RenderContext> render_context = nullptr;
    sge::LLGLResource<LLGL::Texture> texture = nullptr;
    sge::LLGLResource<LLGL::Buffer> vertex_buffer = nullptr;

    StaticLightMapChunk() = default;
    StaticLightMapChunk(std::shared_ptr<sge::RenderContext> context, glm::uvec2 index, const LightMap& lightmap);

    StaticLightMapChunk(StaticLightMapChunk&&) = default;
    StaticLightMapChunk& operator=(StaticLightMapChunk&&) = default;

    void update_texture(glm::uvec2 source_offset, uint32_t source_stride, glm::uvec2 texture_offset, glm::uvec2 size, Color* colors);

    ~StaticLightMapChunk();
};

class RenderChunk {
public:
    RenderChunk(
        std::shared_ptr<class GameRenderer> renderer,
        glm::uvec2 index,
        const glm::vec2& world_pos,
        const WorldData& world,
        ChunkInstance* block_data_arena,
        ChunkInstance* wall_data_arena
    ) :
        m_renderer(std::move(renderer)),
        m_world_pos(world_pos * Constants::RENDER_CHUNK_SIZE),
        m_index(index)
    {
        build_mesh(world, block_data_arena, wall_data_arena);
    }

    RenderChunk(RenderChunk&&) = default;
    RenderChunk& operator=(RenderChunk&&) = default;

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
        return m_block_buffer_array.get();
    }

    [[nodiscard]]
    inline LLGL::BufferArray* wall_buffer_array() const noexcept {
        return m_wall_buffer_array.get();
    }

    [[nodiscard]]
    inline LLGL::Buffer* block_instance_buffer() const noexcept {
        return m_block_instance_buffer.get();
    }

    [[nodiscard]]
    inline LLGL::Buffer* wall_instance_buffer() const noexcept {
        return m_wall_instance_buffer.get();
    }

    ~RenderChunk();

private:
    std::shared_ptr<class GameRenderer> m_renderer = nullptr;

    glm::vec2 m_world_pos;
    glm::uvec2 m_index;
    sge::LLGLResource<LLGL::BufferArray> m_block_buffer_array = nullptr;
    sge::LLGLResource<LLGL::BufferArray> m_wall_buffer_array = nullptr;
    sge::LLGLResource<LLGL::Buffer> m_wall_instance_buffer = nullptr;
    sge::LLGLResource<LLGL::Buffer> m_block_instance_buffer = nullptr;
    uint16_t m_block_count = 0;
    uint16_t m_wall_count = 0;
    bool m_blocks_dirty = false;
    bool m_walls_dirty = false;
};

#endif