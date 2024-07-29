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

namespace UI {
    void Update(Inventory& inventory);
    void Render(const Camera& camera, const Inventory& inventory);
};

#endif