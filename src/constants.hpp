#ifndef TERRARIA_CONSTANTS_HPP
#define TERRARIA_CONSTANTS_HPP

#pragma once

#include <stdint.h>

namespace Constants {
    constexpr double FIXED_UPDATE_INTERVAL = 1.0 / 60.0;
    constexpr float TILE_SIZE = 16.0f;
    constexpr float WALL_SIZE = 32.0f;
    constexpr float PARTICLE_SIZE = 8.0f;

    constexpr float CAMERA_MAX_ZOOM = 0.2f;
#if DEBUG
    constexpr float CAMERA_MIN_ZOOM = 5.0f;
#else
    constexpr float CAMERA_MIN_ZOOM = 1.5f;
#endif

    constexpr float TILE_TEXTURE_PADDING = 2.0f;
    constexpr float WALL_TEXTURE_PADDING = 4.0f;

    constexpr float RENDER_CHUNK_SIZE = 50.0f;
    constexpr uint32_t RENDER_CHUNK_SIZE_U = 50u;
    constexpr int SUBDIVISION = 4;
    
    constexpr float LightDecay(bool solid) {
        if constexpr (SUBDIVISION == 8) {
            return solid ? 0.86 : 0.975;
        } else if (SUBDIVISION == 4) {
            return solid ? 0.78 : 0.91;
        }

        return solid ? 0.56 : 0.91;
    }
};

#endif