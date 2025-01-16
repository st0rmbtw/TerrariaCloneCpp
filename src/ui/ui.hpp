#ifndef UI_HPP
#define UI_HPP

#pragma once

#include "../renderer/camera.h"
#include "../player/inventory.hpp"
#include "../player/player.hpp"

namespace UI {
    void Init();
    void FixedUpdate();
    void PreUpdate(Inventory& inventory);
    void Update(Inventory& inventory);
    void PostUpdate();
    void Draw(const Camera& camera, const Player& player);
};

#endif