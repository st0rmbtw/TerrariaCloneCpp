#pragma once

#ifndef TERRARIA_UI_HPP
#define TERRARIA_UI_HPP

#include "renderer/camera.h"

namespace UI {
    void Update();
    void Render(const Camera& camera);
};

#endif