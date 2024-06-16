#pragma once

#ifndef TERRARIA_TILE_POS_HPP
#define TERRARIA_TILE_POS_HPP

#include <stdint.h>
#include <glm/glm.hpp>
#include "constants.hpp"

enum class TileOffset: uint8_t {
    Top,
    Bottom,
    Left,
    Right,
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight
};

struct TilePos {
    int x, y;

    TilePos() : x(0), y(0) {}

    TilePos(int x, int y) : x(x), y(y) {}
    TilePos(const glm::ivec2& pos) : x(pos.x), y(pos.y) {}

    TilePos(uint32_t x, uint32_t y) : x(static_cast<int>(x)), y(static_cast<int>(y)) {}
    TilePos(const glm::uvec2& pos) : x(static_cast<int>(pos.x)), y(static_cast<int>(pos.y)) {}

    TilePos offset(TileOffset offset) const {
        switch (offset) {
        case TileOffset::Top: 
            return TilePos(this->x, this->y - 1);
        case TileOffset::Bottom:
            return TilePos(this->x, this->y + 1);
        case TileOffset::Left:
            return TilePos(this->x - 1, this->y);
        case TileOffset::Right:
            return TilePos(this->x + 1, this->y);
        case TileOffset::TopLeft:
            return TilePos(this->x - 1, this->y - 1);
        case TileOffset::TopRight:
            return TilePos(this->x + 1, this->y - 1);
        case TileOffset::BottomLeft:
            return TilePos(this->x - 1, this->y + 1);
        case TileOffset::BottomRight:
            return TilePos(this->x + 1, this->y + 1);
        }
    }

    inline static TilePos from_world_pos(const glm::vec2& mouse_pos) {
        return TilePos(glm::ivec2(mouse_pos / Constants::TILE_SIZE));
    }

    inline glm::vec2 to_world_pos(void) const {
        return glm::vec2(x, y) * Constants::TILE_SIZE;
    }

    inline glm::vec2 to_world_pos_center(void) const {
        return glm::vec2(x, y) * Constants::TILE_SIZE + glm::vec2(8.0f);
    }
};

#endif