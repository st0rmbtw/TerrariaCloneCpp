#ifndef TERRARIA_WALL_HPP
#define TERRARIA_WALL_HPP

#pragma once

#include <stdint.h>
#include <stdlib.h>
#include "types/texture_atlas_pos.hpp"

enum class WallType : uint8_t {
    StoneWall = 0,
    DirtWall = 1,
    Count
};

inline constexpr static int16_t wall_hp(WallType /* wall_type */) {
    return 70;
}

struct Wall {
    WallType type;
    int16_t hp;
    uint8_t variant;
    TextureAtlasPos atlas_pos;

    Wall(WallType wall_type) : 
        type(wall_type),
        hp(wall_hp(wall_type)),
        variant(static_cast<uint8_t>(rand() % 3)),
        atlas_pos(0, 0) {}
};

#endif