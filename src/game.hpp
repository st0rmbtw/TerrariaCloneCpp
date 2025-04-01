#ifndef GAME_HPP
#define GAME_HPP

#pragma once

#include <SGE/types/backend.hpp>
#include <SGE/types/config.hpp>

namespace Game {
    bool Init(sge::RenderBackend backend, sge::AppConfig config);
    void Run();
    void Destroy();
};

#endif