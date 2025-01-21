#pragma once

#ifndef BACKGROUND_HPP
#define BACKGROUND_HPP

#include "renderer/camera.h"
#include "world/world.hpp"

namespace Background {
    void SetupWorldBackground(const World& world);
    void SetupMenuBackground();
    
    void Update(const Camera& camera, const World& world);
    void Draw();
};

#endif