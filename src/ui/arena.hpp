#pragma once

#ifndef UI_ARENA_HPP_
#define UI_ARENA_HPP_

#include <cstdint>
#include <cstddef>

#include <SGE/assert.hpp>

class Arena {
public:
    explicit Arena(std::size_t capacity) : m_capacity{ capacity } {
        m_data = new uint8_t[capacity];
    }

    // Allocate raw memory from the arena
    void* allocate(std::size_t size, std::size_t alignment = alignof(std::max_align_t)) noexcept {
        const size_t aligned_offset = (m_current_offset + alignment - 1) & ~(alignment - 1);

        SGE_ASSERT(aligned_offset + size < m_capacity);

        void* ptr = m_data + aligned_offset;
        m_current_offset = aligned_offset + size;
        return ptr;
    }

    inline void clear() {
        m_current_offset = 0;
    }

    ~Arena() {
        delete[] m_data;
    }

private:
    uint8_t* m_data = nullptr;
    size_t m_current_offset = 0;
    size_t m_capacity = 0;
};

#endif