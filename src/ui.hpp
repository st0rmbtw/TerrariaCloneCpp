#ifndef TERRARIA_UI_HPP
#define TERRARIA_UI_HPP

#pragma once

#include "renderer/camera.h"
#include "player/inventory.hpp"

namespace UI {
    void Init();
    void PreUpdate(Inventory& inventory);
    void Update(Inventory& inventory);
    void PostUpdate();
    void Draw(const Camera& camera, const Inventory& inventory);
};

#endif