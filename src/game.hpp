#ifndef GAME_HPP
#define GAME_HPP

#pragma once

#include <SGE/types/backend.hpp>

struct AppConfig {
    bool vsync = false;
    bool fullscreen = false;
    uint8_t samples = 1;
};

namespace Game {
    bool Init(sge::RenderBackend backend, AppConfig config, int16_t world_width, int16_t world_height);
    void Run();
    void Destroy();
};

#endif