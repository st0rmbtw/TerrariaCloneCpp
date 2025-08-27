#pragma once

#ifndef FRAMETIME_HPP_
#define FRAMETIME_HPP_

namespace FrameTime {
    void Update(float frametime);
    float GetAverageFPS();
    float GetCurrentFPS();
};

#endif