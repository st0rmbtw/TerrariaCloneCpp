#include "chunk.hpp"
#include "chunk_manager.hpp"

#include <SGE/profile.hpp>
#include <ranges>

#include "../renderer/renderer.hpp"

#include "lightmap.hpp"
#include "utils.hpp"

using Constants::TILE_SIZE;
using Constants::RENDER_CHUNK_SIZE;
using Constants::LIGHTMAP_CHUNK_TILE_SIZE;

static sge::URect get_chunk_range(const sge::Rect& camera_fov, const glm::uvec2& world_size, glm::uvec2 chunk_size, uint32_t expand = 0) {
    uint32_t left = 0;
    uint32_t right = 0;
    uint32_t bottom = 0;
    uint32_t top = 0;

    if (camera_fov.min.x > TILE_SIZE) {
        left = glm::floor((camera_fov.min.x - TILE_SIZE) / (TILE_SIZE * chunk_size.x));
        if (left >= expand) left -= expand;
    }
    if (camera_fov.max.x > 0.0f) {
        right = glm::ceil((camera_fov.max.x + TILE_SIZE) / (TILE_SIZE * chunk_size.x)) + expand;
    }
    if (camera_fov.min.y > TILE_SIZE) {
        top = glm::floor((camera_fov.min.y - TILE_SIZE) / (TILE_SIZE * chunk_size.y));
        if (top >= expand) top -= expand;
    }
    if (camera_fov.max.y > 0.0f) { 
        bottom = glm::ceil((camera_fov.max.y + TILE_SIZE) / (TILE_SIZE * chunk_size.y)) + expand;
    }

    const glm::uvec2 chunk_max_pos = (world_size + chunk_size - 1u) / chunk_size;

    if (right >= chunk_max_pos.x) right = chunk_max_pos.x;
    if (bottom >= chunk_max_pos.y) bottom = chunk_max_pos.y;

    return {glm::uvec2(left, top), glm::uvec2(right, bottom)};
}

void ChunkManager::manage_render_chunks(const WorldData& world, const sge::Camera& camera) {
    ZoneScoped;

    const sge::Rect camera_fov = utils::get_camera_fov(camera);
    const sge::URect chunk_range = get_chunk_range(camera_fov, world.area.size(), glm::uvec2(Constants::RENDER_CHUNK_SIZE_U));

    m_render_chunks.set_capacity((chunk_range.width() + 2) * (chunk_range.height() + 2));

    for (auto& [key, value] : m_render_chunks) {
        if (value.first.dirty()) {
            value.first.rebuild_mesh(world, m_block_data_arena, m_wall_data_arena);
        }
    }

    m_visible_chunks.clear();

    for (uint32_t y = chunk_range.min.y; y < chunk_range.max.y; ++y) {
        for (uint32_t x = chunk_range.min.x; x < chunk_range.max.x; ++x) {
            const glm::uvec2 chunk_pos = glm::uvec2(x, y);

            m_visible_chunks.insert(chunk_pos);

            if (!m_render_chunks.contains(chunk_pos)) {
                const glm::vec2 world_pos = glm::vec2(x * TILE_SIZE, y * TILE_SIZE);
                m_render_chunks.insert(chunk_pos, RenderChunk(m_renderer, chunk_pos, world_pos, world, m_block_data_arena, m_wall_data_arena));
            }
        }
    }
}

void ChunkManager::manage_light_chunks(const WorldData& world, const sge::Camera& camera) {
    ZoneScoped;

    const sge::Rect camera_fov = utils::get_camera_fov(camera);
    const sge::URect chunk_range = get_chunk_range(camera_fov, world.area.size(), glm::uvec2(LIGHTMAP_CHUNK_TILE_SIZE), 1);

    m_light_chunks.set_capacity((chunk_range.width() + 2) * (chunk_range.height() + 2));

    m_visible_light_chunks.clear();

    for (uint32_t y = chunk_range.min.y; y < chunk_range.max.y; ++y) {
        for (uint32_t x = chunk_range.min.x; x < chunk_range.max.x; ++x) {
            const glm::uvec2 chunk_pos = glm::uvec2(x, y);

            m_visible_light_chunks.insert(chunk_pos);

            if (!m_light_chunks.contains(chunk_pos)) {
                StaticLightMapChunk chunk(m_renderer->GetRenderer()->GetRenderContext(), chunk_pos, world.lightmap);
                m_light_chunks.insert(chunk_pos, std::move(chunk));
            }
        }
    }
}

void ChunkManager::set_blocks_changed(TilePos tile_pos) {
    const glm::uvec2 chunk_pos = glm::uvec2(tile_pos.x, tile_pos.y) / static_cast<uint32_t>(Constants::RENDER_CHUNK_SIZE_U);
    const auto chunk = m_render_chunks.get_unchecked(chunk_pos);
    if (chunk != nullptr) {
        chunk->set_blocks_dirty();
    }
}

void ChunkManager::set_walls_changed(TilePos tile_pos) {
    const glm::uvec2 chunk_pos = glm::uvec2(tile_pos.x, tile_pos.y) / static_cast<uint32_t>(Constants::RENDER_CHUNK_SIZE_U);
    const auto chunk = m_render_chunks.get_unchecked(chunk_pos);
    if (chunk != nullptr) {
        chunk->set_walls_dirty();
    }
}