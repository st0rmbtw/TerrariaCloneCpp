#pragma once

#ifndef UI_HPP_
#define UI_HPP_

#include <cstdint>
#include <glm/vec2.hpp>

#include <SGE/types/sprite.hpp>

namespace ReservedTypeID {
    enum : uint8_t {
        Container = 0,
        Count
    };
}

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
    inline static UiRect top_left(float value) {
        return UiRect(value, 0.0f, value, 0.0f);
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

class Sizing {
public:
    enum class Type : uint8_t {
        Fixed = 0,
        Percent,
        Fit,
        Grow
    };

public:
    Sizing() = default;

    [[nodiscard]]
    inline static Sizing Fixed(float size) {
        Sizing sizing;
        sizing.m_type = Type::Fixed;
        sizing.m_value = size;
        return sizing;
    }

    [[nodiscard]]
    inline static Sizing Percent(float percent) {
        Sizing sizing;
        sizing.m_type = Type::Percent;
        sizing.m_value = percent;
        return sizing;
    }

    [[nodiscard]]
    inline static Sizing Fit() {
        Sizing sizing;
        sizing.m_type = Type::Fit;
        return sizing;
    }

    [[nodiscard]]
    inline static Sizing Grow() {
        Sizing sizing;
        sizing.m_type = Type::Grow;
        return sizing;
    }

    [[nodiscard]]
    inline Type type() const noexcept {
        return m_type;
    }

    [[nodiscard]]
    inline float value() const noexcept {
        return m_value;
    }

private:
    Type m_type = Type::Fit;
    float m_value = 0.0f;
};

class UiSize {
public:
    UiSize() = default;
    UiSize(Sizing width, Sizing height) : m_width(width), m_height(height) {}

    [[nodiscard]]
    inline static UiSize Fixed(float width, float height) {
        return UiSize(Sizing::Fixed(width), Sizing::Fixed(height));
    }

    [[nodiscard]]
    inline static UiSize Fixed(const glm::vec2& size) {
        return UiSize(Sizing::Fixed(size.x), Sizing::Fixed(size.y));
    }

    [[nodiscard]]
    inline static UiSize Percent(float width_percent, float height_percent) {
        return UiSize(Sizing::Percent(width_percent), Sizing::Percent(height_percent));
    }

    [[nodiscard]]
    inline static UiSize Fit() {
        return UiSize(Sizing::Fit(), Sizing::Fit());
    }

    [[nodiscard]]
    inline Sizing width() const noexcept {
        return m_width;
    }

    [[nodiscard]]
    inline Sizing height() const noexcept {
        return m_height;
    }

private:
    Sizing m_width;
    Sizing m_height;
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

class RootDesc {
public:
    RootDesc(const glm::vec2& dimensions) : m_size(dimensions) {}

    RootDesc& with_gap(float gap) noexcept {
        m_gap = gap;
        return *this;
    }

    RootDesc& with_padding(const UiRect& padding) noexcept {
        m_padding = padding;
        return *this;
    }

    inline RootDesc& with_horizontal_alignment(Alignment alignment) noexcept {
        m_horizontal_alignment = alignment;
        return *this;
    }

    inline RootDesc& with_vertical_alignment(Alignment alignment) noexcept {
        m_vertical_alignment = alignment;
        return *this;
    }

    RootDesc& with_orientation(LayoutOrientation orientation) noexcept {
        m_orientation = orientation;
        return *this;
    }

    [[nodiscard]]
    inline LayoutOrientation orientation() const noexcept {
        return m_orientation;
    }

    [[nodiscard]]
    inline Alignment horizontal_alignment() const noexcept {
        return m_horizontal_alignment;
    }

    [[nodiscard]]
    inline Alignment vertical_alignment() const noexcept {
        return m_vertical_alignment;
    }

    [[nodiscard]]
    inline float gap() const noexcept {
        return m_gap;
    }

    [[nodiscard]]
    inline const UiRect& padding() const noexcept {
        return m_padding;
    }

    [[nodiscard]]
    inline const glm::vec2& size() const noexcept {
        return m_size;
    }

private:
    UiRect m_padding{};
    glm::vec2 m_size{ 0.0f, 0.0f };
    float m_gap{ 0.0f };
    LayoutOrientation m_orientation{ LayoutOrientation::Horizontal };
    Alignment m_horizontal_alignment{ Alignment::Start };
    Alignment m_vertical_alignment{ Alignment::Start };
};

class ElementDesc {
public:
    ElementDesc() = default;
    ElementDesc(const UiSize& size) : m_size(size) {}

    inline ElementDesc& with_gap(float gap) noexcept {
        m_gap = gap;
        return *this;
    }

    inline ElementDesc& with_padding(const UiRect& padding) noexcept {
        m_padding = padding;
        return *this;
    }

    inline ElementDesc& with_horizontal_alignment(Alignment alignment) noexcept {
        m_horizontal_alignment = alignment;
        return *this;
    }

    inline ElementDesc& with_vertical_alignment(Alignment alignment) noexcept {
        m_vertical_alignment = alignment;
        return *this;
    }

    inline ElementDesc& with_orientation(LayoutOrientation orientation) noexcept {
        m_orientation = orientation;
        return *this;
    }

    inline ElementDesc& with_size(UiSize size) noexcept {
        m_size = size;
        return *this;
    }

    [[nodiscard]]
    inline LayoutOrientation orientation() const noexcept {
        return m_orientation;
    }

    [[nodiscard]]
    inline Alignment horizontal_alignment() const noexcept {
        return m_horizontal_alignment;
    }

    [[nodiscard]]
    inline Alignment vertical_alignment() const noexcept {
        return m_vertical_alignment;
    }

    [[nodiscard]]
    inline float gap() const noexcept {
        return m_gap;
    }

    [[nodiscard]]
    inline const UiRect& padding() const noexcept {
        return m_padding;
    }

    [[nodiscard]]
    inline const UiSize& size() const noexcept {
        return m_size;
    }

private:
    UiRect m_padding{};
    UiSize m_size{};
    float m_gap{ 0.0f };
    LayoutOrientation m_orientation{ LayoutOrientation::Horizontal };
    Alignment m_horizontal_alignment{ Alignment::Start };
    Alignment m_vertical_alignment{ Alignment::Start };
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

struct UiElement {
    glm::vec2 position = glm::vec2(0.0f);
    glm::vec2 size = glm::vec2(0.0f);

    const void* custom_data = nullptr;
    size_t custom_data_size = 0;

    uint32_t unique_id = 0;
    uint32_t type_id = 0;

    uint32_t z_index = 0;
};

namespace UI {
    void Start(const RootDesc& desc);
    void BeginElement(uint32_t type_id, const ElementDesc& desc, const void* custom_data, size_t custom_data_size);
    
    inline void BeginElement(const ElementDesc& desc) {
        BeginElement(ReservedTypeID::Container, desc, nullptr, 0);
    }
    
    template <typename T>
    inline void BeginElement(uint32_t type_id, const ElementDesc& desc, const T& custom_data) {
        BeginElement(type_id, desc, &custom_data, sizeof(custom_data));
    }

    inline void BeginElement(uint32_t type_id, const ElementDesc& desc) {
        BeginElement(type_id, desc, nullptr, 0);
    }

    void EndElement();

    inline void AddElement(uint32_t type_id, const ElementDesc& desc, const void* custom_data = nullptr, size_t custom_data_size = 0) {
        BeginElement(type_id, desc, custom_data, custom_data_size);
        EndElement();
    }

    template <typename T>
    inline void AddElement(uint32_t type_id, const ElementDesc& desc, const T& custom_data) {
        AddElement(type_id, desc, &custom_data, sizeof(custom_data));
    }

    const std::vector<UiElement>& Finish();

    [[nodiscard]]
    bool IsMouseOverUi();
};

#endif