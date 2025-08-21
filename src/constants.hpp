#pragma once

#ifndef CONSTANTS_HPP_
#define CONSTANTS_HPP_

#include <cstdint>

#include "math/constexpr_math.hpp"

namespace Constants {
    constexpr double FIXED_UPDATE_INTERVAL = 1.0 / 60.0;
    constexpr float TILE_SIZE = 16.0f;
    constexpr float WALL_SIZE = 32.0f;
    constexpr float TORCH_SIZE = 20.0f;
    constexpr float PARTICLE_SIZE = 8.0f;
    constexpr float WORLD_BOUNDARY_OFFSET = 2.0f;
    constexpr std::size_t MAX_PARTICLES_COUNT = 10 * 100000;

    constexpr float CAMERA_MAX_ZOOM = 0.2f;
#if DEBUG
    constexpr float CAMERA_MIN_ZOOM = 5.0f;
#else
    constexpr float CAMERA_MIN_ZOOM = 1.5f;
#endif

    constexpr float TILE_TEXTURE_PADDING = 2.0f;
    constexpr float WALL_TEXTURE_PADDING = 4.0f;

    constexpr float RENDER_CHUNK_SIZE = 50.0f;
    constexpr uint8_t RENDER_CHUNK_SIZE_U = 50u;
    constexpr int SUBDIVISION = 8;
    constexpr float LIGHT_EPSILON = 0.01;

    constexpr float ITEM_GRAB_RANGE = 5.25f * Constants::TILE_SIZE;

    constexpr float LightDecay(bool solid) {
        if constexpr (SUBDIVISION == 8) {
            return solid ? 0.92 : 0.975;
        } else if constexpr (SUBDIVISION == 4) {
            return solid ? 0.84 : 0.942;
        } else {
            return solid ? 0.74 : 0.935;
        }
    }

    namespace internal {
        constexpr int LightDecaySteps(bool solid) {
            return gcem::ceil(gcem::log(LIGHT_EPSILON) / gcem::log(LightDecay(solid)));
        }
    }

    constexpr int LIGHT_SOLID_DECAY_STEPS = internal::LightDecaySteps(true);
    constexpr int LIGHT_AIR_DECAY_STEPS = internal::LightDecaySteps(false);

    static constexpr std::size_t WORLD_MAX_LIGHT_COUNT = 2000;
};

#endif