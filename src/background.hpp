#pragma once

#ifndef TERRARIA_BACKGROUND
#define TERRARIA_BACKGROUND

#include "renderer/camera.h"
#include "world/world.hpp"

namespace Background {
    void SetupWorldBackground(const World& world);
    void SetupMenuBackground();
    
    void Update(const Camera& camera);
    void Render();
};

#endif