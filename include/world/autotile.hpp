#ifndef WORLD_AUTOTILE_HPP
#define WORLD_AUTOTILE_HPP

#pragma once

#include <string>
#include <vector>
#include <list>
#include "types/texture_atlas_pos.hpp"
#include "types/block.hpp"
#include "world/world.hpp"

const uint8_t MERGE_VALIDATION[22][16] = {
    {11, 13, 13, 13, 14, 10, 8, 8, 8, 1, 15, 15, 4, 13, 13, 13},
    {11, 15, 15, 15, 14, 10, 15, 15, 15, 1, 15, 15, 4, 7, 7, 7},
    {11, 7, 7, 7, 14, 10, 15, 15, 15, 1, 15, 15, 4, 11, 11, 11},
    {9, 12, 9, 12, 9, 12, 2, 2, 2, 0, 0, 0, 0, 14, 14, 14},
    {3, 6, 3, 6, 3, 6, 5, 5, 5, 0, 0, 0, 0, 0, 0, 0},
    {15, 15, 15, 15, 11, 14, 8, 10, 15, 15, 15, 15, 15, 0, 0, 0},
    {15, 15, 15, 15, 11, 14, 8, 10, 15, 15, 15, 15, 15, 0, 0, 0},
    {15, 15, 15, 15, 11, 14, 8, 10, 15, 15, 15, 15, 15, 0, 0, 0},
    {15, 15, 15, 15, 11, 14, 2, 10, 15, 15, 15, 15, 15, 0, 0, 0},
    {15, 15, 15, 15, 11, 14, 2, 10, 15, 15, 15, 15, 15, 0, 0, 0},
    {15, 15, 15, 15, 11, 14, 2, 10, 15, 15, 15, 15, 15, 0, 0, 0},
    {13, 13, 13, 13, 13, 13, 15, 15, 15, 5, 5, 5, 0, 0, 0, 0},
    {7, 7, 7, 7, 7, 7, 10, 9, 13, 12, 9, 13, 12, 9, 13, 12},
    {4, 4, 4, 1, 1, 1, 10, 11, 15, 14, 11, 15, 14, 11, 15, 14},
    {5, 5, 5, 5, 5, 5, 10, 3, 7, 6, 3, 7, 6, 3, 7, 6},
    {11, 14, 13, 13, 13, 9, 9, 9, 12, 12, 12, 15, 15, 15, 0, 0},
    {11, 14, 7, 7, 7, 3, 3, 3, 6, 6, 6, 15, 15, 15, 0, 0},
    {11, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 0, 0},
    {13, 13, 13, 13, 13, 13, 15, 15, 15, 0, 0, 0, 0, 0, 0, 0},
    {7, 7, 7, 7, 7, 7, 15, 15, 15, 0, 0, 0, 0, 0, 0, 0},
    {11, 11, 11, 11, 11, 11, 15, 15, 15, 0, 0, 0, 0, 0, 0, 0},
    {14, 14, 14, 14, 14, 14, 15, 15, 15, 0, 0, 0, 0, 0, 0, 0}
};

struct TileRule {

    TileRule() {}

    TileRule(const std::string& start, const std::string& end, uint32_t corner_exclusion_mask = 0, uint32_t blend_exclusion_mask = 0, uint32_t blend_inclusion_mask = 0, uint32_t corner_inclusion_mask = 0);

    [[nodiscard]] bool matches(uint32_t neighbors_mask, uint32_t blend_mask) const;
    [[nodiscard]] bool matches_relaxed(uint32_t neighbors_mask, uint32_t blend_mask) const;

public:
    TextureAtlasPos indexes[3];
private:
    uint32_t corner_exclusion_mask;
    uint32_t blend_exclusion_mask;
    uint32_t blend_inclusion_mask;
    uint32_t corner_inclusion_mask;
};

extern std::vector<std::list<TileRule>> base_rules;
extern std::vector<std::list<TileRule>> blend_rules;
extern std::vector<std::list<TileRule>> grass_rules;

void init_tile_rules();
void update_block_sprite_index(Block& block, const Neighbors<const Block&>& neighbors);
void update_wall_sprite_index(Wall& block, const Neighbors<const Wall&>& neighbors);
void reset_tiles(const TilePos& initial_pos, World& world);

#endif