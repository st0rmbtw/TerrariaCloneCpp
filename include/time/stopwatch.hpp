#pragma once

#ifndef TIME_STOPWATCH_HPP
#define TIME_STOPWATCH_HPP

#include <chrono>


class Stopwatch {
public:
    using Duration = std::chrono::milliseconds;
    
    Stopwatch() :
        m_elapsed(Duration::zero()),
        m_paused(false) {}

    inline Duration elapsed(void) const { return m_elapsed; }
    inline bool paused(void) const { return m_paused; }
    inline float elapsed_secs(void) const { return std::chrono::duration_cast<std::chrono::duration<float>>(m_elapsed).count(); }
    inline double elapsed_secs_d(void) const { return std::chrono::duration_cast<std::chrono::duration<double>>(m_elapsed).count(); }

    void reset(void) { m_elapsed = Duration(); }
    void pause(void) { m_paused = true; }
    void unpause(void) { m_paused = false; }
    
    template <class _Rep, class _Period>
    void set_elapsed(const std::chrono::duration<_Rep, _Period>& elapsed) {
        m_elapsed = std::chrono::duration_cast<Duration>(elapsed);
    }

    template <class _Rep, class _Period>
    void tick(const std::chrono::duration<_Rep, _Period>& delta) {
        if (!m_paused) {
            Duration d = std::chrono::duration_cast<Duration>(delta);
            m_elapsed += d;
        }
    }


private:
    Duration m_elapsed;
    bool m_paused;
};

#endif