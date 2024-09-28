#include "world.hpp"

#include <cstdint>
#include <glm/glm.hpp>
#include <ctime>

#include "../types/block.hpp"
#include "../world/world_gen.h"
#include "../world/autotile.hpp"
#include "../optional.hpp"

void World::set_block(TilePos pos, const Block& block) {
    if (!m_data.is_tilepos_valid(pos)) return;

    const size_t index = m_data.get_tile_index(pos);

    m_data.blocks[index] = block;
    m_changed = true;

    m_data.lightmap_init_tile(pos);

    m_chunk_manager.set_blocks_changed(pos);

    this->update_neighbors(pos);
}

void World::set_block(TilePos pos, BlockType block_type) {
    if (!m_data.is_tilepos_valid(pos)) return;

    const size_t index = m_data.get_tile_index(pos);

    m_data.blocks[index] = Block(block_type);

    reset_tiles(pos, *this);

    update_neighbors(pos);
    
    m_changed = true;

    m_data.lightmap_init_tile(pos);
    m_data.lightmap_blur_area(math::IRect::from_center_half_size(glm::ivec2(pos.x, pos.y), glm::ivec2(16)));

    m_chunk_manager.set_blocks_changed(pos);
}

void World::remove_block(TilePos pos) {
    if (!m_data.is_tilepos_valid(pos)) return;

    const size_t index = m_data.get_tile_index(pos);
    
    if (m_data.blocks[index].is_some()) {
        m_chunk_manager.set_blocks_changed(pos);
    }

    m_data.blocks[index] = tl::nullopt;
    m_changed = true;

    reset_tiles(pos, *this);

    m_data.lightmap_init_tile(pos);
    m_data.lightmap_blur_area(math::IRect::from_center_half_size(glm::ivec2(pos.x, pos.y), glm::ivec2(16)));

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

    m_data.lightmap_init_tile(pos);

    m_chunk_manager.set_walls_changed(pos);
}

void World::generate(uint32_t width, uint32_t height, uint32_t seed) {
    m_data = world_generate(width, height, seed);
}

void World::update(const Camera& camera) {
    m_changed = false;
    m_chunk_manager.manage_chunks(m_data, camera);
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

    if (block.is_some()) {
        const Neighbors<const Block&> neighbors = this->get_block_neighbors(pos);
        const TextureAtlasPos prev_atlas_pos = block->atlas_pos;

        update_block_sprite_index(block.get(), neighbors);
    
        m_chunk_manager.set_blocks_changed(pos);
    }

    if (wall.is_some()) {
        const Neighbors<const Wall&> neighbors = this->get_wall_neighbors(pos);
        const TextureAtlasPos prev_atlas_pos = wall->atlas_pos;

        update_wall_sprite_index(wall.get(), neighbors);
        
        m_chunk_manager.set_walls_changed(pos);
    }
}
