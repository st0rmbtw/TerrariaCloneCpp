#include "chunk_manager.hpp"

#include <tracy/Tracy.hpp>

#include "utils.hpp"

using Constants::TILE_SIZE;
using Constants::RENDER_CHUNK_SIZE;
using Constants::RENDER_CHUNK_SIZE_U;

static sge::URect get_chunk_range(const sge::Rect& camera_fov, const glm::uvec2& world_size, uint32_t expand = 0) {
    uint32_t left = 0;
    uint32_t right = 0;
    uint32_t bottom = 0;
    uint32_t top = 0;

    if (camera_fov.min.x > TILE_SIZE) {
        left = glm::floor((camera_fov.min.x - TILE_SIZE) / (TILE_SIZE * RENDER_CHUNK_SIZE));
        if (left >= expand) left -= expand;
    }
    if (camera_fov.max.x > 0.0f) {
        right = glm::ceil((camera_fov.max.x + TILE_SIZE) / (TILE_SIZE * RENDER_CHUNK_SIZE)) + expand;
    }
    if (camera_fov.min.y > TILE_SIZE) {
        top = glm::floor((camera_fov.min.y - TILE_SIZE) / (TILE_SIZE * RENDER_CHUNK_SIZE));
        if (top >= expand) top -= expand;
    }
    if (camera_fov.max.y > 0.0f) { 
        bottom = glm::ceil((camera_fov.max.y + TILE_SIZE) / (TILE_SIZE * RENDER_CHUNK_SIZE)) + expand;
    }

    const glm::uvec2 chunk_max_pos = (world_size + glm::uvec2(RENDER_CHUNK_SIZE_U)- 1u) / static_cast<uint32_t>(RENDER_CHUNK_SIZE_U);

    if (right >= chunk_max_pos.x) right = chunk_max_pos.x;
    if (bottom >= chunk_max_pos.y) bottom = chunk_max_pos.y;

    return {glm::uvec2(left, top), glm::uvec2(right, bottom)};
}

void ChunkManager::manage_chunks(const WorldData& world, const sge::Camera& camera) {
    ZoneScopedN("ChunkManager::manage_chunks");

    const sge::Rect camera_fov = utils::get_camera_fov(camera);
    const sge::URect chunk_range = get_chunk_range(camera_fov, world.area.size(), 2);
    const sge::URect render_chunk_range = get_chunk_range(camera_fov, world.area.size());

    for (auto it = m_render_chunks.begin(); it != m_render_chunks.end();) {
        if (!chunk_range.contains(it->first)) {
            m_chunks_to_destroy.push_back(it->second);
            it = m_render_chunks.erase(it);
            continue;
        }

        if (it->second.dirty()) {
            it->second.rebuild_mesh(world, m_block_data_arena, m_wall_data_arena);
        }

        it++;
    }

    m_visible_chunks.clear();

    for (uint32_t y = render_chunk_range.min.y; y < render_chunk_range.max.y; ++y) {
        for (uint32_t x = render_chunk_range.min.x; x < render_chunk_range.max.x; ++x) {
            const glm::uvec2 chunk_pos = glm::uvec2(x, y);

            m_visible_chunks.insert(chunk_pos);

            if (m_render_chunks.find(chunk_pos) == m_render_chunks.end()) {
                const glm::vec2 world_pos = glm::vec2(x * TILE_SIZE, y * TILE_SIZE);
                m_render_chunks.emplace(chunk_pos, RenderChunk(chunk_pos, world_pos, world, m_block_data_arena, m_wall_data_arena));
            }
        }
    }
}

void ChunkManager::set_blocks_changed(TilePos tile_pos) {
    const glm::uvec2 chunk_pos = utils::get_chunk_pos(tile_pos);

    const auto chunk = m_render_chunks.find(chunk_pos);
    if (chunk != m_render_chunks.end()) {
        chunk->second.blocks_dirty = true;
    }
}

void ChunkManager::set_walls_changed(TilePos tile_pos) {
    const glm::uvec2 chunk_pos = utils::get_chunk_pos(tile_pos);

    const auto chunk = m_render_chunks.find(chunk_pos);
    if (chunk != m_render_chunks.end()) {
        chunk->second.walls_dirty = true;
    }
}