#ifndef GAME_HPP
#define GAME_HPP

#pragma once

#include "engine/types/backend.hpp"
#include "engine/types/config.hpp"

namespace Game {
    bool Init(RenderBackend backend, GameConfig config);
    void Run();
    void Destroy();
};

#endif