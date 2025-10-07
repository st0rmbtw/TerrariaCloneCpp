#include "world_data.hpp"

#include <cstring>

#include <SGE/defines.hpp>
#include <SGE/profile.hpp>

#include "autotile.hpp"

using Constants::SUBDIVISION;

Block* WorldData::get_block_mut(TilePos pos) {
    if (!is_tilepos_valid(pos)) return nullptr;

    const size_t index = this->get_tile_index(pos);

    if (!this->blocks[index].has_value()) return nullptr;

    return &this->blocks[index].value();
}

Wall* WorldData::get_wall_mut(TilePos pos) {
    if (!is_tilepos_valid(pos)) return nullptr;

    const size_t index = this->get_tile_index(pos);

    if (!this->walls[index].has_value()) return nullptr;
    
    return &this->walls[index].value();
}

std::optional<BlockType> WorldData::get_block_type(TilePos pos) const noexcept {
    const std::optional<Block> block = get_block(pos);
    if (!block.has_value()) return std::nullopt;

    return block->type;
}

std::optional<WallType> WorldData::get_wall_type(TilePos pos) const noexcept {
    const std::optional<Wall> wall = get_wall(pos);
    if (!wall.has_value()) return std::nullopt;

    return wall->type;
}

Neighbors<Block> WorldData::get_block_neighbors(TilePos pos) const {
    return Neighbors<Block> {
        .top = get_block(pos.offset(TileOffset::Top)),
        .bottom = get_block(pos.offset(TileOffset::Bottom)),
        .left = get_block(pos.offset(TileOffset::Left)),
        .right = get_block(pos.offset(TileOffset::Right)),
        .top_left = get_block(pos.offset(TileOffset::TopLeft)),
        .top_right = get_block(pos.offset(TileOffset::TopRight)),
        .bottom_left = get_block(pos.offset(TileOffset::BottomLeft)),
        .bottom_right = get_block(pos.offset(TileOffset::BottomRight)),
    };
}

Neighbors<BlockType> WorldData::get_block_type_neighbors(TilePos pos) const noexcept {
    return Neighbors<BlockType> {
        .top = get_block_type(pos.offset(TileOffset::Top)),
        .bottom = get_block_type(pos.offset(TileOffset::Bottom)),
        .left = get_block_type(pos.offset(TileOffset::Left)),
        .right = get_block_type(pos.offset(TileOffset::Right)),
        .top_left = get_block_type(pos.offset(TileOffset::TopLeft)),
        .top_right = get_block_type(pos.offset(TileOffset::TopRight)),
        .bottom_left = get_block_type(pos.offset(TileOffset::BottomLeft)),
        .bottom_right = get_block_type(pos.offset(TileOffset::BottomRight)),
    };
}

Neighbors<Block*> WorldData::get_block_neighbors_mut(TilePos pos) {
    return Neighbors<Block*> {
        .top = get_block_mut(pos.offset(TileOffset::Top)),
        .bottom = get_block_mut(pos.offset(TileOffset::Bottom)),
        .left = get_block_mut(pos.offset(TileOffset::Left)),
        .right = get_block_mut(pos.offset(TileOffset::Right)),
        .top_left = get_block_mut(pos.offset(TileOffset::TopLeft)),
        .top_right = get_block_mut(pos.offset(TileOffset::TopRight)),
        .bottom_left = get_block_mut(pos.offset(TileOffset::BottomLeft)),
        .bottom_right = get_block_mut(pos.offset(TileOffset::BottomRight)),
    };
}

Neighbors<Wall> WorldData::get_wall_neighbors(TilePos pos) const {
    return Neighbors<Wall> {
        .top = get_wall(pos.offset(TileOffset::Top)),
        .bottom = get_wall(pos.offset(TileOffset::Bottom)),
        .left = get_wall(pos.offset(TileOffset::Left)),
        .right = get_wall(pos.offset(TileOffset::Right)),
        .top_left = get_wall(pos.offset(TileOffset::TopLeft)),
        .top_right = get_wall(pos.offset(TileOffset::TopRight)),
        .bottom_left = get_wall(pos.offset(TileOffset::BottomLeft)),
        .bottom_right = get_wall(pos.offset(TileOffset::BottomRight)),
    };
}

Neighbors<WallType> WorldData::get_wall_type_neighbors(TilePos pos) const noexcept {
    return Neighbors<WallType> {
        .top = get_wall_type(pos.offset(TileOffset::Top)),
        .bottom = get_wall_type(pos.offset(TileOffset::Bottom)),
        .left = get_wall_type(pos.offset(TileOffset::Left)),
        .right = get_wall_type(pos.offset(TileOffset::Right)),
        .top_left = get_wall_type(pos.offset(TileOffset::TopLeft)),
        .top_right = get_wall_type(pos.offset(TileOffset::TopRight)),
        .bottom_left = get_wall_type(pos.offset(TileOffset::BottomLeft)),
        .bottom_right = get_wall_type(pos.offset(TileOffset::BottomRight)),
    };
}

Neighbors<Wall*> WorldData::get_wall_neighbors_mut(TilePos pos) {
    return Neighbors<Wall*> {
        .top = get_wall_mut(pos.offset(TileOffset::Top)),
        .bottom = get_wall_mut(pos.offset(TileOffset::Bottom)),
        .left = get_wall_mut(pos.offset(TileOffset::Left)),
        .right = get_wall_mut(pos.offset(TileOffset::Right)),
        .top_left = get_wall_mut(pos.offset(TileOffset::TopLeft)),
        .top_right = get_wall_mut(pos.offset(TileOffset::TopRight)),
        .bottom_left = get_wall_mut(pos.offset(TileOffset::BottomLeft)),
        .bottom_right = get_wall_mut(pos.offset(TileOffset::BottomRight)),
    };
}

void WorldData::update_tiles_sprites() {
    for (int y = 0; y < area.height(); ++y) {
        for (int x = 0; x < area.width(); ++x) {
            const TilePos pos = TilePos(x, y);
            const uint32_t index = get_tile_index(pos);

            std::optional<Block>& block = blocks[index];
            std::optional<Wall>& wall = walls[index];

            if (block.has_value()) {
                const Neighbors<Block> neighbors = get_block_neighbors(pos);
                update_block_sprite_index(block.value(), neighbors);
            }

            if (wall.has_value()) {
                const Neighbors<Wall> neighbors = get_wall_neighbors(pos);
                update_wall_sprite_index(wall.value(), neighbors);
            }
        }
    }
}