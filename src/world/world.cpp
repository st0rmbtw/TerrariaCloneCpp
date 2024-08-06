#include "world.hpp"

#include <cstdint>
#include <glm/glm.hpp>
#include <ctime>

#include "../types/block.hpp"
#include "../world/world_gen.h"
#include "../world/autotile.hpp"
#include "../math/rect.hpp"
#include "../optional.hpp"

using Constants::TILE_SIZE;
using Constants::RENDER_CHUNK_SIZE_U;
using Constants::RENDER_CHUNK_SIZE;

inline glm::uvec2 get_chunk_pos(TilePos tile_pos) {
    return glm::uvec2(tile_pos.x, tile_pos.y) / RENDER_CHUNK_SIZE_U;
}

tl::optional<const Block&> WorldData::get_block(TilePos pos) const {
    if (!is_tilepos_valid(pos)) return tl::nullopt;

    const size_t index = this->get_tile_index(pos);

    if (this->blocks[index].is_none()) return tl::nullopt;

    return this->blocks[index].get();
}

tl::optional<const Wall&> WorldData::get_wall(TilePos pos) const {
    if (!is_tilepos_valid(pos)) return tl::nullopt;

    const size_t index = this->get_tile_index(pos);

    if (this->walls[index].is_none()) return tl::nullopt;
    
    return this->walls[index].get();
}

tl::optional<Block&> WorldData::get_block_mut(TilePos pos) {
    if (!is_tilepos_valid(pos)) return tl::nullopt;

    const size_t index = this->get_tile_index(pos);

    if (this->blocks[index].is_none()) return tl::nullopt;

    return this->blocks[index].get();
}

tl::optional<Wall&> WorldData::get_wall_mut(TilePos pos) {
    if (!is_tilepos_valid(pos)) return tl::nullopt;

    const size_t index = this->get_tile_index(pos);

    if (this->walls[index].is_none()) return tl::nullopt;
    
    return this->walls[index].get();
}

tl::optional<BlockType> WorldData::get_block_type(TilePos pos) const {
    if (!is_tilepos_valid(pos)) return tl::nullopt;

    tl::optional<const Block&> block = get_block(pos);
    if (block.is_none()) return tl::nullopt;

    return block->type;
}

Neighbors<const Block&> WorldData::get_block_neighbors(TilePos pos) const {
    return Neighbors<const Block&> {
        .top = this->get_block(pos.offset(TileOffset::Top)),
        .bottom = this->get_block(pos.offset(TileOffset::Bottom)),
        .left = this->get_block(pos.offset(TileOffset::Left)),
        .right = this->get_block(pos.offset(TileOffset::Right)),
        .top_left = this->get_block(pos.offset(TileOffset::TopLeft)),
        .top_right = this->get_block(pos.offset(TileOffset::TopRight)),
        .bottom_left = this->get_block(pos.offset(TileOffset::BottomLeft)),
        .bottom_right = this->get_block(pos.offset(TileOffset::BottomRight)),
    };
}

Neighbors<Block&> WorldData::get_block_neighbors_mut(TilePos pos) {
    return Neighbors<Block&> {
        .top = this->get_block_mut(pos.offset(TileOffset::Top)),
        .bottom = this->get_block_mut(pos.offset(TileOffset::Bottom)),
        .left = this->get_block_mut(pos.offset(TileOffset::Left)),
        .right = this->get_block_mut(pos.offset(TileOffset::Right)),
        .top_left = this->get_block_mut(pos.offset(TileOffset::TopLeft)),
        .top_right = this->get_block_mut(pos.offset(TileOffset::TopRight)),
        .bottom_left = this->get_block_mut(pos.offset(TileOffset::BottomLeft)),
        .bottom_right = this->get_block_mut(pos.offset(TileOffset::BottomRight)),
    };
}

Neighbors<const Wall&> WorldData::get_wall_neighbors(TilePos pos) const {
    return Neighbors<const Wall&> {
        .top = this->get_wall(pos.offset(TileOffset::Top)),
        .bottom = this->get_wall(pos.offset(TileOffset::Bottom)),
        .left = this->get_wall(pos.offset(TileOffset::Left)),
        .right = this->get_wall(pos.offset(TileOffset::Right)),
        .top_left = this->get_wall(pos.offset(TileOffset::TopLeft)),
        .top_right = this->get_wall(pos.offset(TileOffset::TopRight)),
        .bottom_left = this->get_wall(pos.offset(TileOffset::BottomLeft)),
        .bottom_right = this->get_wall(pos.offset(TileOffset::BottomRight)),
    };
}

Neighbors<Wall&> WorldData::get_wall_neighbors_mut(TilePos pos) {
    return Neighbors<Wall&> {
        .top = this->get_wall_mut(pos.offset(TileOffset::Top)),
        .bottom = this->get_wall_mut(pos.offset(TileOffset::Bottom)),
        .left = this->get_wall_mut(pos.offset(TileOffset::Left)),
        .right = this->get_wall_mut(pos.offset(TileOffset::Right)),
        .top_left = this->get_wall_mut(pos.offset(TileOffset::TopLeft)),
        .top_right = this->get_wall_mut(pos.offset(TileOffset::TopRight)),
        .bottom_left = this->get_wall_mut(pos.offset(TileOffset::BottomLeft)),
        .bottom_right = this->get_wall_mut(pos.offset(TileOffset::BottomRight)),
    };
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
    if (m_render_chunks.contains(chunk_pos)) {
        m_render_chunks.at(chunk_pos).blocks_dirty = true;
    }

    this->update_neighbors(pos);
}

void World::set_block(TilePos pos, BlockType block_type) {
    if (!m_data.is_tilepos_valid(pos)) return;

    const size_t index = m_data.get_tile_index(pos);

    m_data.blocks[index] = Block(block_type);

    reset_tiles(pos, *this);

    update_neighbors(pos);
    
    m_changed = true;

    const uint8_t pixel = 1;
    // Renderer::state.tiles_texture->set_pixel(index, &pixel);

    const glm::uvec2 chunk_pos = get_chunk_pos(pos);
    if (m_render_chunks.contains(chunk_pos)) {
        m_render_chunks.at(chunk_pos).blocks_dirty = true;
    }
}

void World::remove_block(TilePos pos) {
    if (!m_data.is_tilepos_valid(pos)) return;

    const size_t index = m_data.get_tile_index(pos);
    
    if (m_data.blocks[index].is_some()) {
        const glm::uvec2 chunk_pos = get_chunk_pos(pos);
        if (m_render_chunks.contains(chunk_pos)) {
            m_render_chunks.at(chunk_pos).blocks_dirty = true;
        }
    }

    m_data.blocks[index] = tl::nullopt;
    m_changed = true;

    reset_tiles(pos, *this);

    uint8_t pixel = m_data.wall_exists(pos) ? 2 : 0;

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

    const uint8_t pixel = m_data.block_exists(pos) ? 1 : 2;

    // Renderer::state.tiles_texture->set_pixel(index, &pixel);

    const glm::uvec2 chunk_pos = get_chunk_pos(pos);
    if (m_render_chunks.contains(chunk_pos)) {
        m_render_chunks.at(chunk_pos).walls_dirty = true;
    }
}

void World::generate(uint32_t width, uint32_t height, uint32_t seed) {
    m_data = world_generate(width, height, seed);
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

void World::update(const Camera& camera) {
    m_changed = false;
    manage_chunks(camera);
}

void World::manage_chunks(const Camera& camera) {
    const math::Rect camera_fov = get_camera_fov(camera.position(), camera.get_projection_area());
    const math::URect chunk_range = get_chunk_range(camera_fov, this->area().size(), 2);
    const math::URect render_chunk_range = get_chunk_range(camera_fov, this->area().size());

    for (auto it = m_render_chunks.begin(); it != m_render_chunks.end();) {
        if (!chunk_range.contains(it->first)) {
            m_chunks_to_destroy.push_back(it->second);
            it = m_render_chunks.erase(it);
            continue;
        }

        if (it->second.dirty()) {
            it->second.build_mesh(*this);
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
                m_render_chunks.insert(std::make_pair(chunk_pos, RenderChunk(chunk_pos, world_pos, *this)));
            }
        }
    }
}

void World::update_neighbors(TilePos initial_pos) {
    for (int y = initial_pos.y - 3; y < initial_pos.y + 3; ++y) {
        for (int x = initial_pos.x - 3; x < initial_pos.x + 3; ++x) {
            auto pos = TilePos(x, y);
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

        update_block_sprite_index(block.get(), neighbors);
        
        block_updated = true;
    }

    if (wall.is_some()) {
        const Neighbors<const Wall&> neighbors = this->get_wall_neighbors(pos);
        const TextureAtlasPos prev_atlas_pos = wall->atlas_pos;

        update_wall_sprite_index(wall.get(), neighbors);
        
        wall_updated = true;
    }
    
    // Don't rebuild chunk vao if nothing changed
    if (wall_updated || block_updated) {
        const glm::uvec2 chunk_pos = get_chunk_pos(pos);
        if (m_render_chunks.contains(chunk_pos)) {
            m_render_chunks.at(chunk_pos).blocks_dirty |= block_updated;
            m_render_chunks.at(chunk_pos).walls_dirty |= wall_updated;
        }
    }
}
