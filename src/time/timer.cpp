#include "timer.hpp"
#include "stopwatch.hpp"

void Timer::tick_impl(const duration_t& delta) {
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
            const Duration::Nanos e = Duration::Cast<Duration::Nanos>(elapsed());
            const Duration::Nanos d = Duration::Cast<Duration::Nanos>(duration());

            if (d.count() == 0) {
                m_times_finished_this_tick = UINT32_MAX;
                set_elapsed(Timer::duration_t::zero());
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