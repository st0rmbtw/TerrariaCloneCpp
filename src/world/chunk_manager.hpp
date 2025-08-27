#pragma once

#ifndef WORLD_CHUNK_MANAGER_HPP_
#define WORLD_CHUNK_MANAGER_HPP_

#include <unordered_map>
#include <unordered_set>
#include <deque>
#include <glm/vec2.hpp>

#include <SGE/renderer/camera.hpp>
#include <SGE/utils/alloc.hpp>
#include "../renderer/types.hpp"
#include "../types/tile_pos.hpp"

#include "chunk.hpp"
#include "world_data.hpp"

class ChunkManager {
public:
    using ChunkMap = std::unordered_map<glm::uvec2, RenderChunk>;
    using ChunkPosSet = std::unordered_set<glm::uvec2>;

    ChunkManager() {
        using Constants::RENDER_CHUNK_SIZE_U;
        m_block_data_arena = sge::checked_alloc<ChunkInstance>(RENDER_CHUNK_SIZE_U * RENDER_CHUNK_SIZE_U);
        m_wall_data_arena = sge::checked_alloc<ChunkInstance>(RENDER_CHUNK_SIZE_U * RENDER_CHUNK_SIZE_U);
    }

    void manage_chunks(const WorldData& world, const sge::Camera& camera);

    void set_blocks_changed(TilePos tile_pos);
    void set_walls_changed(TilePos tile_pos);

    inline void destroy_hidden_chunks() {
        std::deque<RenderChunk>& chunks = m_chunks_to_destroy;
        while (!chunks.empty()) {
            chunks.back().destroy();
            chunks.pop_back();
        }
    }

    inline void destroy() {
        for (auto& entry : m_render_chunks) {
            entry.second.destroy();
        }
    }

    [[nodiscard]]
    inline const ChunkMap& render_chunks() const noexcept {
        return m_render_chunks;
    }

    [[nodiscard]]
    inline const ChunkPosSet& visible_chunks() const noexcept {
        return m_visible_chunks;
    }

    [[nodiscard]]
    inline std::deque<RenderChunk>& chunks_to_destroy() noexcept {
        return m_chunks_to_destroy;
    }

    [[nodiscard]]
    inline bool any_chunks_to_destroy() noexcept {
        return !m_chunks_to_destroy.empty();
    }

    ~ChunkManager() {
        free(m_block_data_arena);
        free(m_wall_data_arena);
    }
private:
    ChunkMap m_render_chunks;
    ChunkPosSet m_visible_chunks;
    std::deque<RenderChunk> m_chunks_to_destroy;

    ChunkInstance* m_block_data_arena = nullptr;
    ChunkInstance* m_wall_data_arena = nullptr;
};

#endif