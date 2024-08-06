#pragma once

#ifndef TERRARIA_BACKGROUND
#define TERRARIA_BACKGROUND

#include "renderer/camera.h"
#include "world/world.hpp"

namespace Background {
    void Init(const World& world);
    void Update(const Camera& camera);
    void Render(const Camera& camera);
};

#endif