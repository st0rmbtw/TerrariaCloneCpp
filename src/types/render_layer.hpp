#ifndef TERRARIA_TYPES_RENDER_LAYER
#define TERRARIA_TYPES_RENDER_LAYER

#pragma once

#include <stdint.h>

enum class RenderLayer : uint8_t {
    Main = 0,
    World = 1
};

#endif