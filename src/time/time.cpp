#include "time.hpp"

#include "../constants.hpp"

static struct TimeState {
    delta_time_t delta;
    delta_time_t fixed_delta;
    float elapsed_seconds;
    float fixed_elapsed_seconds;
} state;

const delta_time_t& Time::delta(void) { return state.delta; }
const delta_time_t Time::fixed_delta(void) { return delta_time_t(Constants::FIXED_UPDATE_INTERVAL); }
float Time::delta_seconds(void) { return state.delta.count(); }
float Time::elapsed_seconds(void) { return state.elapsed_seconds; }
float Time::fixed_elapsed_seconds(void) { return state.fixed_elapsed_seconds; }

void Time::advance_by(const delta_time_t& delta) {
    state.delta = delta;
    state.elapsed_seconds += delta.count();
}

void Time::fixed_advance_by(const delta_time_t& fixed_delta) {
    state.fixed_delta = fixed_delta;
    state.fixed_elapsed_seconds += fixed_delta.count();
}