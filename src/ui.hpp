#ifndef TERRARIA_UI_HPP
#define TERRARIA_UI_HPP

#pragma once

#include "renderer/camera.h"
#include "player/inventory.hpp"

constexpr float MIN_CURSOR_SCALE = 1.0;
constexpr float MAX_CURSOR_SCALE = MIN_CURSOR_SCALE + 0.20;
constexpr float INVENTORY_PADDING = 10;
constexpr float HOTBAR_SLOT_SIZE = 40;
constexpr float INVENTORY_SLOT_SIZE = HOTBAR_SLOT_SIZE * 1.1;
constexpr float HOTBAR_SLOT_SIZE_SELECTED = HOTBAR_SLOT_SIZE * 1.3;
constexpr float INVENTORY_CELL_MARGIN = 4;

enum class AnimationDirection: uint8_t {
    Backward = 0,
    Forward = 1,
};

enum class UiElement : uint8_t {
    HotbarCell,
    InventoryCell
};

class Element {
public:
    enum : uint8_t {
        None = 0,
        Hovered = 1 << 0,
        Pressed = 1 << 1
    };

    Element(UiElement element_type, int data, const math::Rect& rect) :
        m_data(data),
        m_element_type(element_type),
        m_rect(rect) {}

    inline void press() noexcept { m_state = m_state | Pressed; }
    inline void hover() noexcept { m_state = m_state | Hovered; }

    [[nodiscard]] inline bool none() const noexcept { return m_state == None; }
    [[nodiscard]] inline bool hovered() const noexcept { return m_state & Hovered; }
    [[nodiscard]] inline bool pressed() const noexcept { return m_state & Pressed; }

    [[nodiscard]] inline const math::Rect& rect() const noexcept { return m_rect; }
    [[nodiscard]] inline UiElement type() const noexcept { return m_element_type; }
    [[nodiscard]] inline int data() const noexcept { return m_data; }
private:
    uint8_t m_state = 0;
    UiElement m_element_type;
    int m_data = 0;
    math::Rect m_rect;
};

namespace UI {
    void Init();
    void PreUpdate(Inventory& inventory);
    void Update(Inventory& inventory);
    void PostUpdate();
    void Render(const Camera& camera, const Inventory& inventory);
};

#endif