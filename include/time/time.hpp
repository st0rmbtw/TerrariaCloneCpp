#ifndef TERRARIA_TIME_TIME_HPP
#define TERRARIA_TIME_TIME_HPP

#pragma once

#include "common.h"
#include "constants.hpp"

using delta_time_t = std::chrono::duration<double>;

namespace Time {
    const delta_time_t& delta();
    const delta_time_t fixed_delta();
    float delta_seconds();
    float elapsed_seconds();

    void advance_by(const delta_time_t& delta);
};

#endif