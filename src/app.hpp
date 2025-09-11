#pragma once

#ifndef APP_HPP_
#define APP_HPP_

#include <SGE/types/backend.hpp>
#include <glm/vec2.hpp>

struct AppConfig {
    bool vsync = false;
    bool fullscreen = false;
    uint8_t samples = 1;
};

namespace App {
    bool Init(sge::RenderBackend backend, AppConfig config, int16_t world_width, int16_t world_height);
    void Run();
    void Destroy();

    glm::uvec2 GetWindowResolution();
};

#endif