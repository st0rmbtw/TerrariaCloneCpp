#pragma once

#ifndef TERRARIA_TEXTURE_ATLAS_POS_HPP
#define TERRARIA_TEXTURE_ATLAS_POS_HPP

#include <stdint.h>

struct TextureAtlasPos {
    uint16_t x, y;

    TextureAtlasPos() {}
    TextureAtlasPos(uint16_t x, uint16_t y) : x(x), y(y) {}

    bool operator==(const TextureAtlasPos& rhs) {
        return this->x == rhs.x & this->y == rhs.y;
    }

    bool operator!=(const TextureAtlasPos& rhs) {
        return !(this->x == rhs.x & this->y == rhs.y);
    }
};

#endif