#ifndef WORLD_CHUNK_MANAGER_HPP
#define WORLD_CHUNK_MANAGER_HPP

#pragma once

#include <unordered_map>
#include <unordered_set>
#include <deque>
#include <glm/vec2.hpp>

#include "../renderer/camera.h"
#include "../types/tile_pos.hpp"

#include "chunk.hpp"
#include "world_data.hpp"

class ChunkManager {
public:
    void manage_chunks(const WorldData& world, const Camera& camera);

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

    [[nodiscard]] inline const std::unordered_map<glm::uvec2, RenderChunk>& render_chunks() const { return m_render_chunks; }
    [[nodiscard]] inline const std::unordered_set<glm::uvec2>& visible_chunks() const { return m_visible_chunks; }
    [[nodiscard]] inline std::deque<RenderChunk>& chunks_to_destroy() { return m_chunks_to_destroy; }
    [[nodiscard]] inline bool any_chunks_to_destroy() { return !m_chunks_to_destroy.empty(); }
private:
    std::unordered_map<glm::uvec2, RenderChunk> m_render_chunks;
    std::unordered_set<glm::uvec2> m_visible_chunks;
    std::deque<RenderChunk> m_chunks_to_destroy;
};

#endif