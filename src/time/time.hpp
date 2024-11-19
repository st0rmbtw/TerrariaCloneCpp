#ifndef TERRARIA_TIME_TIME_HPP
#define TERRARIA_TIME_TIME_HPP

#pragma once

#include <chrono>

using delta_time_t = std::chrono::duration<float>;

namespace Time {
    const delta_time_t& delta();
    const delta_time_t fixed_delta();
    float delta_seconds();
    float elapsed_seconds();
    float fixed_delta_seconds();
    float fixed_elapsed_seconds();

    void set_fixed_timestep_seconds(const float seconds);

    template <class Rep, class Period>
    void set_fixed_timestep(const std::chrono::duration<Rep, Period>& delta) {
        const delta_time_t d = std::chrono::duration_cast<delta_time_t>(delta);
        set_fixed_timestep_seconds(d.count());
    }
    
    void advance_by(const delta_time_t& delta);
    void fixed_advance();
};

#endif