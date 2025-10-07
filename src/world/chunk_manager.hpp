#pragma once

#include "SGE/utils/containers/swapbackvector.hpp"
#ifndef WORLD_CHUNK_MANAGER_HPP_
#define WORLD_CHUNK_MANAGER_HPP_

#include <glm/vec2.hpp>

#include <SGE/renderer/camera.hpp>
#include <SGE/utils/alloc.hpp>

#include "../renderer/types.hpp"
#include "../types/tile_pos.hpp"
#include "../utils/data/lru_cache.hpp"
#include "../utils/thread_pool/thread_pool.hpp"

#include "chunk.hpp"
#include "world_data.hpp"

template <>
struct std::less<glm::uvec2> {
    bool operator()(const glm::uvec2& a, const glm::uvec2& b) const noexcept {
        return std::hash<glm::uvec2>{}(a) < std::hash<glm::uvec2>{}(b);
    }
};

class ChunkManager {
    struct LightChunkTaskResult {
        LightMap lightmap;
        glm::uvec2 index;
    };

public:
    using RenderChunks = LRUCache<glm::uvec2, RenderChunk>;
    using LightChunks = LRUCache<glm::uvec2, StaticLightMapChunk>;

    ChunkManager() {
        using Constants::RENDER_CHUNK_SIZE_U;
        using Constants::LIGHTMAP_CHUNK_SIZE;
        m_block_data_arena = sge::checked_alloc<ChunkInstance>(RENDER_CHUNK_SIZE_U * RENDER_CHUNK_SIZE_U);
        m_wall_data_arena = sge::checked_alloc<ChunkInstance>(RENDER_CHUNK_SIZE_U * RENDER_CHUNK_SIZE_U);
        m_color_arena = sge::checked_alloc<Color>(LIGHTMAP_CHUNK_SIZE * LIGHTMAP_CHUNK_SIZE);
    }

    void preload_chunks(const WorldData& world, const sge::Camera& camera);

    void manage_chunks(const WorldData& world, const sge::Camera& camera) {
        manage_render_chunks(world, camera);
        manage_light_chunks(world, camera);
    }

    void set_blocks_changed(TilePos tile_pos);
    void set_walls_changed(TilePos tile_pos);

    [[nodiscard]]
    inline const RenderChunks& render_chunks() const noexcept {
        return m_render_chunks;
    }

    [[nodiscard]]
    inline const LightChunks& light_chunks() const noexcept {
        return m_light_chunks;
    }

    [[nodiscard]]
    inline const std::unordered_set<glm::uvec2>& visible_chunks() const noexcept {
        return m_visible_chunks;
    }

    [[nodiscard]]
    inline const std::unordered_set<glm::uvec2>& visible_light_chunks() const noexcept {
        return m_visible_light_chunks;
    }

    ~ChunkManager() {
        free(m_block_data_arena);
        free(m_wall_data_arena);
        free(m_color_arena);
    }
private:
    void manage_render_chunks(const WorldData& world, const sge::Camera& camera);
    void manage_light_chunks(const WorldData& world, const sge::Camera& camera);
    
    void preload_render_chunks(const WorldData& world, const sge::Camera& camera) {
        manage_render_chunks(world, camera);
    }
    void preload_light_chunks(const WorldData& world, const sge::Camera& camera);

private:
    dp::thread_pool<> m_thread_pool{ 4 };
    
    RenderChunks m_render_chunks{ 5 };
    LightChunks m_light_chunks{ 10 };

    std::unordered_set<glm::uvec2> m_queued_light_chunks;

    std::mutex m_mutex;

    sge::SwapbackVector<std::future<LightChunkTaskResult>> m_light_chunk_tasks;

    std::unordered_set<glm::uvec2> m_visible_chunks;
    std::unordered_set<glm::uvec2> m_visible_light_chunks;

    ChunkInstance* m_block_data_arena = nullptr;
    ChunkInstance* m_wall_data_arena = nullptr;
    Color* m_color_arena = nullptr;
};

#endif