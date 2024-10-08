#include "chunk_manager.hpp"

using Constants::TILE_SIZE;

inline glm::uvec2 get_chunk_pos(TilePos tile_pos) {
    return glm::uvec2(tile_pos.x, tile_pos.y) / RENDER_CHUNK_SIZE_U;
}

inline math::Rect get_camera_fov(const glm::vec2& camera_pos, const math::Rect& projection_area) {
    return {camera_pos + projection_area.min, camera_pos + projection_area.max};
}

math::URect get_chunk_range(const math::Rect& camera_fov, const glm::uvec2& world_size, uint32_t expand = 0) {
    uint32_t left = 0;
    uint32_t right = 0;
    uint32_t bottom = 0;
    uint32_t top = 0;

    if (camera_fov.min.x > 0.0f) {
        left = glm::floor(camera_fov.min.x / (TILE_SIZE * RENDER_CHUNK_SIZE));
        if (left >= expand) left -= expand;
    }
    if (camera_fov.max.x > 0.0f) {
        right = glm::ceil(camera_fov.max.x / (TILE_SIZE * RENDER_CHUNK_SIZE)) + expand;
    }
    if (camera_fov.min.y > 0.0f) {
        top = glm::floor(camera_fov.min.y / (TILE_SIZE * RENDER_CHUNK_SIZE));
        if (top >= expand) top -= expand;
    }
    if (camera_fov.max.y > 0.0f) { 
        bottom = glm::ceil(camera_fov.max.y / (TILE_SIZE * RENDER_CHUNK_SIZE)) + expand;
    }

    const glm::uvec2 chunk_max_pos = (world_size + RENDER_CHUNK_SIZE_U - 1u) / RENDER_CHUNK_SIZE_U;

    if (right >= chunk_max_pos.x) right = chunk_max_pos.x;
    if (bottom >= chunk_max_pos.y) bottom = chunk_max_pos.y;

    return {glm::uvec2(left, top), glm::uvec2(right, bottom)};
}

void ChunkManager::manage_chunks(const WorldData& world, const Camera& camera) {
    const math::Rect camera_fov = get_camera_fov(camera.position(), camera.get_projection_area());
    const math::URect chunk_range = get_chunk_range(camera_fov, world.area.size(), 2);
    const math::URect render_chunk_range = get_chunk_range(camera_fov, world.area.size());

    for (auto it = m_render_chunks.begin(); it != m_render_chunks.end();) {
        if (!chunk_range.contains(it->first)) {
            m_chunks_to_destroy.push_back(it->second);
            it = m_render_chunks.erase(it);
            continue;
        }

        if (it->second.dirty()) {
            it->second.build_mesh(world);
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
                m_render_chunks.emplace(chunk_pos, RenderChunk(chunk_pos, world_pos, world));
            }
        }
    }
}

void ChunkManager::set_blocks_changed(TilePos tile_pos) {
    const glm::uvec2 chunk_pos = get_chunk_pos(tile_pos);
    if (m_render_chunks.contains(chunk_pos)) {
        m_render_chunks.at(chunk_pos).blocks_dirty = true;
    }
}

void ChunkManager::set_walls_changed(TilePos tile_pos) {
    const glm::uvec2 chunk_pos = get_chunk_pos(tile_pos);
    if (m_render_chunks.contains(chunk_pos)) {
        m_render_chunks.at(chunk_pos).walls_dirty = true;
    }
}