#ifndef TERRARIA_TILE_POS_HPP
#define TERRARIA_TILE_POS_HPP

#pragma once

#include <stdint.h>
#include <glm/glm.hpp>
#include "../constants.hpp"

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

    constexpr TilePos() : x(0), y(0) {}

    constexpr TilePos(int x, int y) : x(x), y(y) {}
    constexpr TilePos(const glm::ivec2& pos) : x(pos.x), y(pos.y) {}

    constexpr TilePos(uint32_t x, uint32_t y) : x(static_cast<int>(x)), y(static_cast<int>(y)) {}
    constexpr TilePos(const glm::uvec2& pos) : x(static_cast<int>(pos.x)), y(static_cast<int>(pos.y)) {}

    [[nodiscard]] 
    constexpr TilePos offset(TileOffset offset) const {
        switch (offset) {
        case TileOffset::Top: 
            return {this->x, this->y - 1};
        case TileOffset::Bottom:
            return {this->x, this->y + 1};
        case TileOffset::Left:
            return {this->x - 1, this->y};
        case TileOffset::Right:
            return {this->x + 1, this->y};
        case TileOffset::TopLeft:
            return {this->x - 1, this->y - 1};
        case TileOffset::TopRight:
            return {this->x + 1, this->y - 1};
        case TileOffset::BottomLeft:
            return {this->x - 1, this->y + 1};
        case TileOffset::BottomRight:
            return {this->x + 1, this->y + 1};
        }
    }

    inline static TilePos from_world_pos(const glm::vec2& mouse_pos) {
        return {glm::ivec2(mouse_pos / Constants::TILE_SIZE)};
    }

    [[nodiscard]] inline glm::vec2 to_world_pos() const {
        return glm::vec2(x, y) * Constants::TILE_SIZE;
    }

    [[nodiscard]]
    inline glm::vec2 to_world_pos_center() const {
        return glm::vec2(x, y) * Constants::TILE_SIZE + glm::vec2(8.0f);
    }

    constexpr inline TilePos operator/(int d) const {
        return TilePos(x / d, y / d);
    }
};

#endif