#pragma once

#ifndef BACKGROUND_HPP
#define BACKGROUND_HPP

#include <SGE/renderer/camera.hpp>
#include "world/world.hpp"

namespace Background {
    void SetupWorldBackground(const World& world);
    void SetupMenuBackground();
    
    void Update(const sge::Camera& camera, const World& world);
    void Draw();
};

#endif