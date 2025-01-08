#ifndef TYPES_TEXTURE_ATLAS_POS_HPP
#define TYPES_TEXTURE_ATLAS_POS_HPP

#pragma once

#include <stdint.h>

struct TextureAtlasPos {
    uint16_t x, y;

    TextureAtlasPos() = default;
    TextureAtlasPos(uint16_t x, uint16_t y) : x(x), y(y) {}

    bool operator==(const TextureAtlasPos& rhs) const {
        return this->x == rhs.x && this->y == rhs.y;
    }

    bool operator!=(const TextureAtlasPos& rhs) const {
        return this->x != rhs.x && this->y != rhs.y;
    }
};

#endif