#pragma once

#ifndef WORLD_CHUNK_MANAGER_HPP_
#define WORLD_CHUNK_MANAGER_HPP_

#include <glm/vec2.hpp>

#include <SGE/renderer/camera.hpp>
#include <SGE/utils/alloc.hpp>

#include "../renderer/types.hpp"
#include "../types/tile_pos.hpp"
#include "../data_structure/lru_cache.hpp"

#include "chunk.hpp"
#include "world_data.hpp"

template <>
struct std::less<glm::uvec2> {
    bool operator()(const glm::uvec2& a, const glm::uvec2& b) const noexcept {
        return std::hash<glm::uvec2>{}(a) < std::hash<glm::uvec2>{}(b);
    }
};

class ChunkManager {
public:
    ChunkManager() {
        using Constants::RENDER_CHUNK_SIZE_U;
        m_block_data_arena = sge::checked_alloc<ChunkInstance>(RENDER_CHUNK_SIZE_U * RENDER_CHUNK_SIZE_U);
        m_wall_data_arena = sge::checked_alloc<ChunkInstance>(RENDER_CHUNK_SIZE_U * RENDER_CHUNK_SIZE_U);
    }

    void manage_chunks(const WorldData& world, const sge::Camera& camera) {
        manage_render_chunks(world, camera);
        manage_light_chunks(world, camera);
    }

    void set_blocks_changed(TilePos tile_pos);
    void set_walls_changed(TilePos tile_pos);

    [[nodiscard]]
    inline const LRUCache<glm::uvec2, RenderChunk>& render_chunks() const noexcept {
        return m_render_chunks;
    }

    [[nodiscard]]
    inline const LRUCache<glm::uvec2, StaticLightMapChunk>& light_chunks() const noexcept {
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
    }
private:
    void manage_render_chunks(const WorldData& world, const sge::Camera& camera);
    void manage_light_chunks(const WorldData& world, const sge::Camera& camera);

private:
    LRUCache<glm::uvec2, RenderChunk> m_render_chunks{ 5 };
    LRUCache<glm::uvec2, StaticLightMapChunk> m_light_chunks{ 5 };

    std::unordered_set<glm::uvec2> m_visible_chunks;
    std::unordered_set<glm::uvec2> m_visible_light_chunks;

    ChunkInstance* m_block_data_arena = nullptr;
    ChunkInstance* m_wall_data_arena = nullptr;
};

#endif