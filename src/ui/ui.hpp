#ifndef UI_HPP
#define UI_HPP

#pragma once

#include <SGE/renderer/camera.hpp>

#include "../player/inventory.hpp"
#include "../player/player.hpp"

namespace UI {
    void Init() noexcept;
    void FixedUpdate() noexcept;
    void PreUpdate(Inventory& inventory) noexcept;
    void Update(Inventory& inventory) noexcept;
    void PostUpdate() noexcept;
    void Draw(const sge::Camera& camera, const Player& player) noexcept;
};

#endif