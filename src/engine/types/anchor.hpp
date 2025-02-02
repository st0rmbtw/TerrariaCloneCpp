#ifndef _ENGINE_TYPES_ANCHOR_HPP_
#define _ENGINE_TYPES_ANCHOR_HPP_

#pragma once

#include <stdint.h>
#include <glm/vec2.hpp>

class Anchor {
public:
    enum Value : uint8_t {
        Center = 0,
        TopLeft,
        TopRight,
        TopCenter,
        BottomLeft,
        BottomRight,
        BottomCenter
    };

    Anchor() = default;
    constexpr Anchor(Value backend) : m_value(backend) {}

    constexpr operator Value() const { return m_value; }
    explicit operator bool() const = delete;

    inline glm::vec2 operator*(const glm::vec2& vec) const noexcept {
        return to_vec2() * vec;
    }

    [[nodiscard]] constexpr inline glm::vec2 to_vec2() const noexcept {
        switch (m_value) {
        case Anchor::Center:       return {0.5f, 0.5f};
        case Anchor::TopLeft:      return {0.0f, 0.0f};
        case Anchor::TopRight:     return {1.0f, 0.0f};
        case Anchor::BottomLeft:   return {0.0f, 1.0f};
        case Anchor::BottomRight:  return {1.0f, 1.0f};
        case Anchor::TopCenter:    return {0.5f, 0.0f};
        case Anchor::BottomCenter: return {0.5f, 1.0f};
        }
    }

private:
    Value m_value = Value::Center;
};

#endif