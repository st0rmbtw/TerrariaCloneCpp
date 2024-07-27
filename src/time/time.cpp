#include "time/time.hpp"

static struct TimeState {
    delta_time_t delta;
    float elapsed_seconds;
} state;

const delta_time_t& Time::delta(void) { return state.delta; }
const delta_time_t Time::fixed_delta(void) { return delta_time_t(Constants::FIXED_UPDATE_INTERVAL); }
float Time::delta_seconds(void) { return state.delta.count(); }
float Time::elapsed_seconds(void) { return state.elapsed_seconds; }

void Time::advance_by(const delta_time_t& delta) {
    state.delta = delta;
    state.elapsed_seconds += delta.count();
}