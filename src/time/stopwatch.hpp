#ifndef TIME_STOPWATCH_HPP
#define TIME_STOPWATCH_HPP

#pragma once

#include <chrono>

class Stopwatch {
public:
    using Duration = std::chrono::duration<float, std::milli>;
    
    Stopwatch() : m_elapsed(Duration::zero()) {}

    [[nodiscard]] inline Duration elapsed() const { return m_elapsed; }
    [[nodiscard]] inline bool paused() const { return m_paused; }
    [[nodiscard]] inline float elapsed_secs() const { return std::chrono::duration_cast<std::chrono::duration<float>>(m_elapsed).count(); }

    inline void reset() { m_elapsed = Duration(); }
    inline void pause() { m_paused = true; }
    inline void unpause() { m_paused = false; }
    
    template <class Rep, class Period>
    inline void set_elapsed(const std::chrono::duration<Rep, Period>& elapsed) {
        m_elapsed = std::chrono::duration_cast<Duration>(elapsed);
    }

    template <class Rep, class Period>
    inline void tick(const std::chrono::duration<Rep, Period>& delta) {
        if (!m_paused) {
            Duration d = std::chrono::duration_cast<Duration>(delta);
            m_elapsed += d;
        }
    }

private:
    Duration m_elapsed;
    bool m_paused = false;
};

#endif