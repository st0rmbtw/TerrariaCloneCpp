#ifndef TYPES_WALL_HPP
#define TYPES_WALL_HPP

#pragma once

#include <stdint.h>
#include <stdlib.h>

#include "texture_atlas_pos.hpp"

enum class WallType : uint8_t {
    StoneWall = 0,
    DirtWall = 1,
    WoodWall = 4,
};

inline constexpr static int16_t wall_hp(WallType /* wall_type */) {
    return 70;
}

struct Wall {
    TextureAtlasPos atlas_pos;
    int16_t hp;
    WallType type;
    uint8_t variant;

    Wall(WallType wall_type) :
        atlas_pos(0, 0),
        hp(wall_hp(wall_type)),
        type(wall_type),
        variant(static_cast<uint8_t>(rand() % 3)) {}
};

#endif