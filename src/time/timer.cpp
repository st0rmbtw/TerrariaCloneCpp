#include "timer.hpp"

#include <chrono>

void Timer::tick_impl(const Duration& delta) {
    if (paused()) {
        m_times_finished_this_tick = 0;
        if (m_mode == TimerMode::Repeating) {
            m_finished = false;
        }
        return;
    }

    if (m_mode != TimerMode::Repeating && finished()) {
        m_times_finished_this_tick = 0;
        return;
    }

    m_stopwatch.tick(delta);
    m_finished = elapsed() >= duration();

    if (finished()) {
        if (m_mode == TimerMode::Repeating) {
            const std::chrono::nanoseconds e = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed());
            std::chrono::nanoseconds d = std::chrono::duration_cast<std::chrono::nanoseconds>(duration());

            if (d.count() == 0) {
                m_times_finished_this_tick = UINT32_MAX;
                set_elapsed(Timer::Duration::zero());
            } else {
                m_times_finished_this_tick = e / d;
                set_elapsed(e % d);
            }

        } else {
            m_times_finished_this_tick = 1;
            set_elapsed(duration());
        }
    } else {
        m_times_finished_this_tick = 0;
    }
}