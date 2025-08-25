#include "frametime.hpp"
#include "SGE/time/time.hpp"

#include <LLGL/Container/DynamicArray.h>

static constexpr uint16_t FRAMETIME_RECORD_MAX_COUNT = 120;

static struct State {
    LLGL::DynamicArray<float> frametime_records{ FRAMETIME_RECORD_MAX_COUNT };
    uint16_t frametime_record_index = 0;
    float frametime_record_sum = 0.0f;
} state;

void FrameTime::Update(float frametime) {
    state.frametime_record_sum -= state.frametime_records[state.frametime_record_index];
    state.frametime_record_sum += frametime;
    state.frametime_records[state.frametime_record_index] = frametime;
    state.frametime_record_index = (state.frametime_record_index + 1) % FRAMETIME_RECORD_MAX_COUNT;
}

float FrameTime::GetAverageFPS() {
    return (1.0f / (state.frametime_record_sum / FRAMETIME_RECORD_MAX_COUNT));
}

float FrameTime::GetCurrentFPS() {
    const float dt = sge::Time::DeltaSeconds();
    if (dt > 0.0f)
        return 1.0f / dt;

    return 0.0f;
}