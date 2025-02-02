#ifndef _ENGINE_TYPES_RENDER_LAYER_HPP_
#define _ENGINE_TYPES_RENDER_LAYER_HPP_

#pragma once

#include <stdint.h>

enum class RenderLayer : uint8_t {
    Main = 0,
    World = 1
};

#endif