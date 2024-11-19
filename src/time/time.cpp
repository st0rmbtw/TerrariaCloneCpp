#include "time.hpp"

static struct TimeState {
    delta_time_t delta;
    delta_time_t fixed_delta;
    float elapsed_seconds;
    float fixed_elapsed_seconds;
} state;

const delta_time_t& Time::delta(void) { return state.delta; }
float Time::delta_seconds(void) { return state.delta.count(); }
float Time::elapsed_seconds(void) { return state.elapsed_seconds; }
const delta_time_t Time::fixed_delta(void) { return state.fixed_delta; }
float Time::fixed_delta_seconds(void) { return state.fixed_delta.count(); }
float Time::fixed_elapsed_seconds(void) { return state.fixed_elapsed_seconds; }

void Time::set_fixed_timestep_seconds(const float seconds) {
    state.fixed_delta = delta_time_t(seconds);
}

void Time::advance_by(const delta_time_t& delta) {
    state.delta = delta;
    state.elapsed_seconds += delta.count();
}

void Time::fixed_advance() {
    state.fixed_elapsed_seconds += state.fixed_delta.count();
}