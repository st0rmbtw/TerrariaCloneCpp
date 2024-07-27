// Based on https://github.com/bevyengine/bevy/blob/main/crates/bevy_time/src/timer.rs

#pragma once

#ifndef TIME_TIMER_HPP
#define TIME_TIMER_HPP

#include <chrono>
#include <cstdint>
#include "stopwatch.hpp"

namespace duration {
    using seconds = std::chrono::seconds;
    using seconds_float = std::chrono::duration<float>;
}

enum class TimerMode : uint8_t {
    Once = 0,
    Repeating = 1
};

class Timer {
public:
    using Duration = std::chrono::milliseconds;
    Timer() :
        m_stopwatch(),
        m_duration(Duration::zero()),
        m_mode(TimerMode::Once),
        m_finished(false),
        m_times_finished_this_tick(0) {}

    template <class _Rep, class _Period>
    Timer(const std::chrono::duration<_Rep, _Period>& duration, TimerMode mode) : 
        m_duration(std::chrono::duration_cast<Duration>(duration)),
        m_mode(mode) {}

    inline bool finished(void) const { return m_finished; }
    inline bool just_finished(void) const { return m_times_finished_this_tick > 0; }
    inline Duration duration(void) const { return m_duration; }
    inline Duration elapsed(void) const { return m_stopwatch.elapsed(); }
    inline float elapsed_secs(void) const { return m_stopwatch.elapsed_secs(); }
    inline bool paused(void) const { return m_stopwatch.paused(); }
    inline uint32_t times_finished_this_tick(void) const { return m_times_finished_this_tick; }

    inline void pause(void) { m_stopwatch.pause(); }
    inline void unpause(void) { m_stopwatch.unpause(); }
    inline void reset(void) {
        m_stopwatch.reset();
        m_finished = false;
        m_times_finished_this_tick = 0;
    }
    
    template <class _Rep, class _Period>
    inline void set_elapsed(const std::chrono::duration<_Rep, _Period>& elapsed) {
        m_stopwatch.set_elapsed(elapsed);
    }

    template <class _Rep, class _Period>
    inline void set_duration(const std::chrono::duration<_Rep, _Period>& duration) {
        const Duration d = std::chrono::duration_cast<Duration>(duration);
        m_duration = duration;
    }

    template <class _Rep, class _Period>
    const Timer& tick(const std::chrono::duration<_Rep, _Period>& delta) {
        const Duration d = std::chrono::duration_cast<Duration>(delta);
        tick_impl(d);
        return *this;
    }

private:
    void tick_impl(const Duration& delta);

private:
    Stopwatch m_stopwatch;
    Duration m_duration;
    TimerMode m_mode;
    bool m_finished;
    uint32_t m_times_finished_this_tick;
};

#endif