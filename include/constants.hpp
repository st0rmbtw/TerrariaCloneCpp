#pragma once

#ifndef TERRARIA_CONSTANTS_HPP
#define TERRARIA_CONSTANTS_HPP

#include <stdint.h>

namespace Constants {
    constexpr double FIXED_UPDATE_INTERVAL = 1.0 / 60.0;
    constexpr float TILE_SIZE = 16.0f;
    constexpr float WALL_SIZE = 32.0f;

    constexpr float CAMERA_MAX_ZOOM = 0.2f;
#if DEBUG
    constexpr float CAMERA_MIN_ZOOM = 10.0f;
#else
    constexpr float CAMERA_MIN_ZOOM = 1.5f;
#endif

    constexpr uint32_t MAX_TILE_TEXTURE_WIDTH = 287;
    constexpr uint32_t MAX_TILE_TEXTURE_HEIGHT = 395;
    constexpr float TILE_TEXTURE_PADDING = 2.0f;

    constexpr uint32_t MAX_WALL_TEXTURE_WIDTH = 468;
    constexpr uint32_t MAX_WALL_TEXTURE_HEIGHT = 180;
    constexpr float WALL_TEXTURE_PADDING = 4.0f;
};

#endif