#ifndef WORLD_AUTOTILE_HPP
#define WORLD_AUTOTILE_HPP

#pragma once

#include "../types/block.hpp"
#include "world.hpp"

void init_tile_rules();
void update_block_sprite_index(Block& block, const Neighbors<Block>& neighbors);
void update_wall_sprite_index(Wall& block, const Neighbors<Wall>& neighbors);
void reset_tiles(const TilePos& initial_pos, World& world);

#endif