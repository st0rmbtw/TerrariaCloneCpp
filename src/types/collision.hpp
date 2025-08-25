#pragma once

#ifndef TYPES_COLLISION_HPP_
#define TYPES_COLLISION_HPP_

#include <SGE/utils/bitflags.hpp>
#include <cstdint>

class Collision {
private:
    enum class Type : uint8_t {
        Up = 1 << 0,
        Down = 1 << 1,
        Left = 1 << 2,
        Right = 1 << 3
    };

public:

    inline void set_up(bool value) noexcept {
        m_flags.set(Type::Up, value);
    }

    inline void set_down(bool value) noexcept {
        m_flags.set(Type::Down, value);
    }

    inline void set_left(bool value) noexcept {
        m_flags.set(Type::Left, value);
    }

    inline void set_right(bool value) noexcept {
        m_flags.set(Type::Right, value);
    }

    inline void reset() noexcept {
        m_flags.reset();
    }

    [[nodiscard]]
    inline bool up() const noexcept {
        return m_flags[Type::Up];
    }

    [[nodiscard]]
    inline bool down() const noexcept {
        return m_flags[Type::Down];
    }

    [[nodiscard]]
    inline bool left() const noexcept {
        return m_flags[Type::Left];
    }

    [[nodiscard]]
    inline bool right() const noexcept {
        return m_flags[Type::Right];
    }

private:
    sge::BitFlags<Type> m_flags;
};

#endif