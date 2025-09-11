#pragma once

#ifndef UI_HPP_
#define UI_HPP_

#include <cstdint>
#include <glm/vec2.hpp>

#include <SGE/types/sprite.hpp>

class UiRect {
public:
    UiRect() = default;

    UiRect(float left, float right, float top, float bottom) :
        m_left(left),
        m_right(right),
        m_top(top),
        m_bottom(bottom) {}

    [[nodiscard]]
    inline static UiRect horizontal(float value) {
        return UiRect(value, value, 0.0f, 0.0f);
    }

    [[nodiscard]]
    inline static UiRect vertical(float value) {
        return UiRect(0.0f, 0.0f, value, value);
    }

    [[nodiscard]]
    inline float left() const noexcept {
        return m_left;
    }

    [[nodiscard]]
    inline float right() const noexcept {
        return m_right;
    }

    [[nodiscard]]
    inline float top() const noexcept {
        return m_top;
    }

    [[nodiscard]]
    inline float bottom() const noexcept {
        return m_bottom;
    }

private:
    float m_left = 0.0f;
    float m_right = 0.0f;
    float m_top = 0.0f;
    float m_bottom = 0.0f;
};

enum class LayoutOrientation : uint8_t {
    Horizontal = 0,
    Vertical = 1
};

enum class Alignment : uint8_t {
    Start = 0,
    Center = 1,
    End = 2
};

class LayoutDesc {
public:
    LayoutDesc(LayoutOrientation orientation) : m_orientation(orientation) {};

    LayoutDesc& with_gap(float gap) noexcept {
        m_gap = gap;
        return *this;
    }

    LayoutDesc& with_padding(const UiRect& padding) noexcept {
        m_padding = padding;
        return *this;
    }

    LayoutDesc& with_alignment(Alignment alignment) noexcept {
        m_alignment = alignment;
        return *this;
    }

    [[nodiscard]]
    inline LayoutOrientation orientation() const noexcept {
        return m_orientation;
    }

    [[nodiscard]]
    inline Alignment alignment() const noexcept {
        return m_alignment;
    }

    [[nodiscard]]
    inline float gap() const noexcept {
        return m_gap;
    }

    [[nodiscard]]
    inline const UiRect& padding() const noexcept {
        return m_padding;
    }

private:
    UiRect m_padding = UiRect();
    float m_gap = 0.0f;
    LayoutOrientation m_orientation = LayoutOrientation::Horizontal;
    Alignment m_alignment = Alignment::Start;
};

namespace Interaction {
    using Type = uint8_t;

    enum : Type {
        None = 0,
        Pressed = (1 << 0),
        Hovered = (1 << 1)
    };
}

struct ElementState {
    glm::vec2 position = glm::vec2(0.0f);
    Interaction::Type interaction = Interaction::None;

    [[nodiscard]]
    inline bool pressed() const noexcept {
        return (interaction & Interaction::Pressed) != 0;
    }

    [[nodiscard]]
    inline bool hovered() const noexcept {
        return (interaction & Interaction::Hovered) != 0;
    }
};

namespace UI {
    void BeginLayout(const LayoutDesc& layout);

    ElementState Element(const glm::vec2& size);

    void EndLayout();

    [[nodiscard]]
    bool IsMouseOverUi();
};

#endif