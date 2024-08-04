#ifndef TERRARIA_ANCHOR_HPP
#define TERRARIA_ANCHOR_HPP

#pragma once

#include <stdint.h>
#include <glm/vec2.hpp>

enum class Anchor : uint8_t {
    Center = 0,
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight
};

[[nodiscard]]
constexpr inline glm::vec2 anchor_to_vec2(Anchor anchor) noexcept {
    switch (anchor) {
    case Anchor::Center:      return {0.5f, 0.5f};
    case Anchor::TopLeft:     return {0.0f, 0.0f};
    case Anchor::TopRight:    return {1.0f, 0.0f};
    case Anchor::BottomLeft:  return {0.0f, 1.0f};
    case Anchor::BottomRight: return {1.0f, 1.0f};
    }
}

inline glm::vec2 operator*(const glm::vec2& vec, Anchor anchor) noexcept {
    return anchor_to_vec2(anchor) * vec;
}

#endif