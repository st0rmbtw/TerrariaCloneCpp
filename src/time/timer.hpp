// Based on https://github.com/bevyengine/bevy/blob/main/crates/bevy_time/src/timer.rs

#ifndef TIME_TIMER_HPP
#define TIME_TIMER_HPP

#pragma once

#include <chrono>
#include <cstdint>
#include "stopwatch.hpp"

enum class TimerMode : uint8_t {
    Once = 0,
    Repeating = 1
};

class Timer {
public:
    using Duration = Stopwatch::Duration;

    Timer() :
        m_duration(Duration::zero()),
        m_times_finished_this_tick(0),
        m_mode(TimerMode::Once),
        m_finished(false) {}

    template <class Rep, class Period>
    Timer(const std::chrono::duration<Rep, Period>& duration, TimerMode mode) : 
        m_duration(std::chrono::duration_cast<Duration>(duration)),
        m_times_finished_this_tick(0),
        m_mode(mode),
        m_finished(false) {}

    inline static Timer from_seconds(float seconds, TimerMode mode) {
        return Timer(std::chrono::duration<float>(seconds), mode);
    }

    [[nodiscard]] inline bool finished() const { return m_finished; }
    [[nodiscard]] inline bool just_finished() const { return m_times_finished_this_tick > 0; }
    [[nodiscard]] inline Duration duration() const { return m_duration; }
    [[nodiscard]] inline Duration elapsed() const { return m_stopwatch.elapsed(); }
    [[nodiscard]] inline float elapsed_secs() const { return m_stopwatch.elapsed_secs(); }
    [[nodiscard]] inline bool paused() const { return m_stopwatch.paused(); }
    [[nodiscard]] inline uint32_t times_finished_this_tick() const { return m_times_finished_this_tick; }

    inline void pause() { m_stopwatch.pause(); }
    inline void unpause() { m_stopwatch.unpause(); }
    inline void reset() {
        m_stopwatch.reset();
        m_finished = false;
        m_times_finished_this_tick = 0;
    }
    
    template <class Rep, class Period>
    inline void set_elapsed(const std::chrono::duration<Rep, Period>& elapsed) {
        m_stopwatch.set_elapsed(elapsed);
    }

    template <class Rep, class Period>
    inline void set_duration(const std::chrono::duration<Rep, Period>& duration) {
        m_duration = std::chrono::duration_cast<Duration>(duration);
    }

    template <class Rep, class Period>
    const Timer& tick(const std::chrono::duration<Rep, Period>& delta) {
        const auto d = std::chrono::duration_cast<Duration>(delta);
        tick_impl(d);
        return *this;
    }

    constexpr void set_finished() { set_elapsed(m_duration); }

private:
    void tick_impl(const Duration& delta);

private:
    Stopwatch m_stopwatch;
    Duration m_duration;
    uint32_t m_times_finished_this_tick;
    TimerMode m_mode;
    bool m_finished;
};

#endif