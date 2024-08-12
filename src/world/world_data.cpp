#include "world_data.hpp"

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