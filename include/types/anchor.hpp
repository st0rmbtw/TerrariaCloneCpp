#pragma once

#ifndef TERRARIA_ANCHOR_HPP
#define TERRARIA_ANCHOR_HPP

#include <stdint.h>
#include <glm/glm.hpp>
#include "common.h"

enum class Anchor : uint8_t {
    Center,
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight
};

constexpr static inline glm::vec2 anchor_to_vec2(Anchor anchor) {
    switch (anchor) {
    case Anchor::Center: return glm::vec2(0.5, 0.5);
    case Anchor::TopLeft: return glm::vec2(0., 0.);
    case Anchor::TopRight: return glm::vec2(1., 0.);
    case Anchor::BottomLeft: return glm::vec2(0., 1.);
    case Anchor::BottomRight: return glm::vec2(1., 1.);
    default: UNREACHABLE()
    }
}

inline glm::vec2 operator*(const glm::vec2& vec, const Anchor& anchor) {
    return anchor_to_vec2(anchor) * vec;
}

#endif