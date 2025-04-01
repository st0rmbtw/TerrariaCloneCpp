#ifndef UI_HPP
#define UI_HPP

#pragma once

#include <SGE/renderer/camera.hpp>

#include "../player/inventory.hpp"
#include "../player/player.hpp"

namespace UI {
    void Init();
    void FixedUpdate();
    void PreUpdate(Inventory& inventory);
    void Update(Inventory& inventory);
    void PostUpdate();
    void Draw(const sge::Camera& camera, const Player& player);
};

#endif