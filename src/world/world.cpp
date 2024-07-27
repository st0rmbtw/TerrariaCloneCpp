#include <stdint.h>
#include <glm/glm.hpp>
#include <utility>
#include <time.h>

#include "world/world.hpp"
#include "types/block.hpp"
#include "world/world_gen.h"
#include "world/autotile.hpp"
#include "math/rect.hpp"
#include "assets.hpp"
#include "optional.hpp"

inline glm::uvec2 get_chunk_pos(TilePos tile_pos) {
    return glm::uvec2(tile_pos.x, tile_pos.y) / RENDER_CHUNK_SIZE_U;
}

void World::set_block(TilePos pos, const Block& block) {
    if (!m_data.is_tilepos_valid(pos)) return;

    const size_t index = m_data.get_tile_index(pos);

    m_data.blocks[index] = block;
    m_changed = true;

    // if (Renderer::state.tiles_texture != nullptr) {
    //     const uint8_t pixel[] = {1};
    //     Renderer::state.tiles_texture->set_pixel(index, pixel);
    // }

    const glm::uvec2 chunk_pos = get_chunk_pos(pos);
    if (m_render_chunks.count(chunk_pos) > 0) {
        m_render_chunks.at(chunk_pos).blocks_dirty = true;
    }

    this->update_neighbors(pos);
}

void World::set_block(TilePos pos, BlockType block_type) {
    if (!m_data.is_tilepos_valid(pos)) return;

    const size_t index = m_data.get_tile_index(pos);

    m_data.blocks[index] = Block(block_type);
    Block& block = m_data.blocks[index].value();

    reset_tiles(pos, *this);

    update_neighbors(pos);
    
    m_changed = true;

    const uint8_t pixel = 1;
    // Renderer::state.tiles_texture->set_pixel(index, &pixel);

    const glm::uvec2 chunk_pos = get_chunk_pos(pos);
    if (m_render_chunks.count(chunk_pos) > 0) {
        m_render_chunks.at(chunk_pos).blocks_dirty = true;
    }
}

void World::remove_block(TilePos pos) {
    if (!m_data.is_tilepos_valid(pos)) return;

    const size_t index = m_data.get_tile_index(pos);
    
    if (m_data.blocks[index].is_some()) {
        const glm::uvec2 chunk_pos = get_chunk_pos(pos);
        if (m_render_chunks.count(chunk_pos) > 0) {
            m_render_chunks.at(chunk_pos).blocks_dirty = true;
        }
    }

    m_data.blocks[index] = tl::nullopt;
    m_changed = true;

    reset_tiles(pos, *this);

    uint8_t pixel = 0;
    if (m_data.wall_exists(pos)) {
        pixel = 2;
    }

    // Renderer::state.tiles_texture->set_pixel(index, &pixel);

    this->update_neighbors(pos);
}

void World::update_block_type(TilePos pos, BlockType new_type) {
    const size_t index = m_data.get_tile_index(pos);
    m_data.blocks[index]->type = new_type;
    m_changed = true;
    reset_tiles(pos, *this);
    update_neighbors(pos);
}

void World::set_wall(TilePos pos, WallType wall_type) {
    if (!m_data.is_tilepos_valid(pos)) return;

    const size_t index = m_data.get_tile_index(pos);

    m_data.walls[index] = Wall(wall_type);
    m_changed = true;

    update_neighbors(pos);

    uint8_t pixel = 2;
    if (m_data.block_exists(pos)) {
        pixel = 1;
    }

    // Renderer::state.tiles_texture->set_pixel(index, &pixel);

    const glm::uvec2 chunk_pos = get_chunk_pos(pos);
    if (m_render_chunks.count(chunk_pos) > 0) {
        m_render_chunks.at(chunk_pos).walls_dirty = true;
    }
}

void World::generate(uint32_t width, uint32_t height, size_t seed) {
    WorldData world_data = world_generate(width, height, seed);
    m_data = world_data;
}

inline math::Rect get_camera_fov(const glm::vec2& camera_pos, const math::Rect& projection_area) {
    return math::Rect(camera_pos + projection_area.min, camera_pos + projection_area.max);
}

math::URect get_chunk_range(const math::Rect& camera_fov, const glm::uvec2& world_size) {
    uint32_t left = 0;
    uint32_t right = 0;
    uint32_t bottom = 0;
    uint32_t top = 0;

    if (camera_fov.min.x > 0.0f) {
        left = glm::floor(camera_fov.min.x / (Constants::TILE_SIZE * RENDER_CHUNK_SIZE));
    }
    if (camera_fov.max.x > 0.0f) {
        right = glm::ceil(camera_fov.max.x / (Constants::TILE_SIZE * RENDER_CHUNK_SIZE));
    }
    if (camera_fov.min.y > 0.0f) {
        top = glm::floor(camera_fov.min.y / (Constants::TILE_SIZE * RENDER_CHUNK_SIZE));
    }
    if (camera_fov.max.y > 0.0f) { 
        bottom = glm::ceil(camera_fov.max.y / (Constants::TILE_SIZE * RENDER_CHUNK_SIZE));
    }

    const glm::uvec2 chunk_max_pos = (world_size + RENDER_CHUNK_SIZE_U - 1u) / RENDER_CHUNK_SIZE_U;

    if (right >= chunk_max_pos.x) right = chunk_max_pos.x;
    if (bottom >= chunk_max_pos.y) bottom = chunk_max_pos.y;

    return math::URect(glm::uvec2(left, top), glm::uvec2(right, bottom));
}

void World::update(const Camera& camera) {
    m_changed = false;
    manage_chunks(camera);
}

void World::manage_chunks(const Camera& camera) {
    const math::Rect camera_fov = get_camera_fov(camera.position(), camera.get_projection_area());
    const math::URect chunk_range = get_chunk_range(camera_fov, this->area().size());

    for (auto it = m_render_chunks.begin(); it != m_render_chunks.end();) {
        if (!chunk_range.contains(it->first)) {
            it = m_render_chunks.erase(it);
            continue;
        }

        if (it->second.dirty()) {
            it->second.build_mesh(*this);
        }

        it++;
    }

    for (uint32_t y = chunk_range.min.y; y < chunk_range.max.y; ++y) {
        for (uint32_t x = chunk_range.min.x; x < chunk_range.max.x; ++x) {
            const glm::uvec2 chunk_pos = glm::uvec2(x, y);

            if (m_render_chunks.count(chunk_pos) == 0) {
                const glm::vec2 world_pos = glm::vec2(x * Constants::TILE_SIZE, y * Constants::TILE_SIZE);
                m_render_chunks.insert(std::make_pair(chunk_pos, RenderChunk(chunk_pos, world_pos, *this)));
            }
        }
    }
}

void World::update_neighbors(TilePos initial_pos) {
    for (int y = initial_pos.y - 3; y < initial_pos.y + 3; ++y) {
        for (int x = initial_pos.x - 3; x < initial_pos.x + 3; ++x) {
            TilePos pos = TilePos(x, y);
            update_tile_sprite_index(pos.offset(TileOffset::Left));
            update_tile_sprite_index(pos.offset(TileOffset::Right));
            update_tile_sprite_index(pos.offset(TileOffset::Top));
            update_tile_sprite_index(pos.offset(TileOffset::Bottom));
        }
    }
}

void World::update_tile_sprite_index(TilePos pos) {
    tl::optional<Block&> block = this->get_block_mut(pos);
    tl::optional<Wall&> wall = this->get_wall_mut(pos);

    bool block_updated = false;
    bool wall_updated = false;

    if (block.is_some()) {
        const Neighbors<const Block&> neighbors = this->get_block_neighbors(pos);
        const TextureAtlasPos prev_atlas_pos = block->atlas_pos;

        update_block_sprite_index(block.value(), neighbors);
        
        block_updated = true;
    }

    if (wall.is_some()) {
        const Neighbors<const Wall&> neighbors = this->get_wall_neighbors(pos);
        const TextureAtlasPos prev_atlas_pos = wall->atlas_pos;

        update_wall_sprite_index(wall.value(), neighbors);
        
        wall_updated = true;
    }
    
    // Don't rebuild chunk vao if nothing changed
    if (wall_updated || block_updated) {
        const glm::uvec2 chunk_pos = get_chunk_pos(pos);
        if (m_render_chunks.count(chunk_pos) > 0) {
            m_render_chunks.at(chunk_pos).blocks_dirty |= block_updated;
            m_render_chunks.at(chunk_pos).walls_dirty |= wall_updated;
        }
    }
}
