#pragma once

#ifndef TERRARIA_TIME_TIME_HPP
#define TERRARIA_TIME_TIME_HPP

#include "common.h"
#include "constants.hpp"

using delta_time_t = std::chrono::duration<double>;

namespace Time {
    const delta_time_t& delta(void);
    const delta_time_t fixed_delta(void);
    float delta_seconds(void);
    float elapsed_seconds(void);

    void advance_by(const delta_time_t& delta);
};

#endif