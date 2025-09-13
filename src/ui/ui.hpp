#pragma once

#ifndef UI_HPP_
#define UI_HPP_

#include <cstddef>
#include <cstdint>
#include <optional>
#include <functional>
#include <glm/vec2.hpp>
#include <SGE/input.hpp>
#include <SGE/types/rich_text.hpp>
#include <SGE/types/font.hpp>

class UiRect {
public:
    UiRect() = default;
    UiRect(float left, float right, float top, float bottom) :
        m_left(left),
        m_right(right),
        m_top(top),
        m_bottom(bottom) {}

    [[nodiscard]]
    inline static UiRect Horizontal(float value) {
        return UiRect(value, value, 0.0f, 0.0f);
    }

    [[nodiscard]]
    inline static UiRect Vertical(float value) {
        return UiRect(0.0f, 0.0f, value, value);
    }

    [[nodiscard]]
    inline static UiRect TopLeft(float value) {
        return UiRect(value, 0.0f, value, 0.0f);
    }

    [[nodiscard]]
    inline static UiRect All(float value) {
        return UiRect(value, value, value, value);
    }

    [[nodiscard]]
    inline static UiRect axes(float horizontal, float vertical) {
        return UiRect(horizontal, horizontal, vertical, vertical);
    }

    [[nodiscard]]
    inline static UiRect axes(glm::vec2 axes) {
        return UiRect(axes.x, axes.x, axes.y, axes.y);
    }

    inline UiRect& top(float value) {
        m_top = value;
        return *this;
    }

    inline UiRect& bottom(float value) {
        m_bottom = value;
        return *this;
    }

    inline UiRect& left(float value) {
        m_left = value;
        return *this;
    }

    inline UiRect& right(float value) {
        m_right = value;
        return *this;
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
        Fill
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
    inline static Sizing Fill() {
        Sizing sizing;
        sizing.m_type = Type::Fill;
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
    inline static UiSize Width(Sizing sizing) {
        return UiSize(sizing, Sizing::Fixed(0.0f));
    }

    [[nodiscard]]
    inline static UiSize Height(Sizing sizing) {
        return UiSize(Sizing::Fixed(0.0f), sizing);
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
    inline static UiSize Fill() {
        return UiSize(Sizing::Fill(), Sizing::Fill());
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
    Vertical,
    Stack
};

enum class Alignment : uint8_t {
    Start = 0,
    Center,
    End
};

class RootDesc {
public:
    RootDesc(const glm::vec2 dimensions) : m_size(dimensions) {}

    RootDesc& with_gap(float gap) noexcept {
        m_gap = gap;
        return *this;
    }

    RootDesc& with_padding(const UiRect padding) noexcept {
        m_padding = padding;
        return *this;
    }

    inline RootDesc& with_horizontal_alignment(const Alignment alignment) noexcept {
        m_horizontal_alignment = alignment;
        return *this;
    }

    inline RootDesc& with_vertical_alignment(const Alignment alignment) noexcept {
        m_vertical_alignment = alignment;
        return *this;
    }

    RootDesc& with_orientation(const LayoutOrientation orientation) noexcept {
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
    inline const UiRect padding() const noexcept {
        return m_padding;
    }

    [[nodiscard]]
    inline const glm::vec2 size() const noexcept {
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

struct ElementID {
    std::string_view string_id{};
    uint32_t id = 0;
    uint32_t offset = 0;
    uint32_t base_id = 0;
};

template <>
struct std::hash<ElementID> {
    std::size_t operator()(const ElementID& id) const noexcept {
        return id.id;
    }
};

inline bool operator==(const ElementID& a, const ElementID& b) noexcept {
    return a.id == b.id;
}

struct ElementDesc {
    ElementID id{};
    UiRect padding{};
    UiSize size{};
    float gap{ 0.0f };
    LayoutOrientation orientation{ LayoutOrientation::Stack };
    std::optional<Alignment> self_alignment{ std::nullopt };
    Alignment horizontal_alignment{ Alignment::Start };
    Alignment vertical_alignment{ Alignment::Start };
};

struct TextElementDesc {
    std::optional<Alignment> self_alignment{ std::nullopt };
};

struct TextData {
    const sge::Font& font;
    const sge::RichTextSection* sections;
    size_t sections_count;
};

struct UiElement {
    ElementID unique_id{};

    glm::vec2 position = glm::vec2(0.0f);
    glm::vec2 size = glm::vec2(0.0f);

    const void* custom_data = nullptr;
    size_t custom_data_size = 0;

    const TextData* text_data = nullptr;

    uint32_t type_id = 0;
    uint32_t z_index = 0;
};

namespace UI {
    void Update();

    void Start(const RootDesc& desc);
    void BeginElement(uint32_t type_id, const ElementDesc& desc, bool render = true);
    void EndElement();

    void Text(uint32_t type_id, const sge::Font& font, const sge::RichTextSection* sections, const size_t count, const TextElementDesc desc = {});
    
    void OnClick(std::function<void(sge::MouseButton)>&& on_press);

    void SetCustomData(const void* custom_data, size_t custom_data_size, size_t custom_data_alignment = alignof(std::max_align_t));

    template <uint32_t TypeID, size_t Size>
    inline void Text(const sge::Font& font, const sge::RichText<Size>& text, const TextElementDesc desc = {}) {
        Text(TypeID, font, text.sections().data(), text.size(), desc);
    }

    inline void BeginContainer(const ElementDesc& desc) {
        BeginElement(0, desc, false);
    }

    template <uint32_t TypeID>
    inline void BeginElement(const ElementDesc& desc, bool render = true) {
        BeginElement(TypeID, desc, render);
    }

    inline void Spacer(const UiSize size = UiSize::Fill()) {
        BeginContainer(ElementDesc{ .size = size });
        EndElement();
    }

    template <uint32_t TypeID>
    inline void AddElement(const ElementDesc& desc) {
        BeginElement(TypeID, desc);
        EndElement();
    }

    template <uint32_t TypeID, typename T>
    inline void AddElement(const ElementDesc& desc, const T& custom_data) {
        BeginElement(TypeID, desc);
        SetCustomData(&custom_data, sizeof(custom_data), alignof(T));
        EndElement();
    }

    template <typename T>
    inline void SetCustomData(const T& custom_data) {
        SetCustomData(&custom_data, sizeof(custom_data), alignof(T));
    }

    template <typename F>
    inline void Container(const ElementDesc& desc, F&& children) {
        BeginContainer(desc);
            std::forward<F>(children)();
        EndElement();
    }

    template <uint32_t TypeID, typename F>
    inline void Element(const ElementDesc& desc, F&& children) {
        BeginElement(TypeID, desc);
            std::forward<F>(children)();
        EndElement();
    }

    [[nodiscard]]
    bool IsHovered();

    [[nodiscard]]
    uint32_t GetParentID();

    [[nodiscard]]
    const ElementID& GetElementID();

    [[nodiscard]]
    const std::vector<UiElement>& Finish();

    [[nodiscard]]
    bool IsMouseOverUi();
};

namespace ID {
    [[nodiscard]]
    ElementID Local(const std::string_view key);

    [[nodiscard]]
    ElementID Global(const std::string_view key, uint32_t index = 0);
};

#endif
