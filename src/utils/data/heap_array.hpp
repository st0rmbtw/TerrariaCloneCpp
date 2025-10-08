#ifndef UTILS_DATA_HEAP_ARRAY_
#define UTILS_DATA_HEAP_ARRAY_

#include <SGE/utils/alloc.hpp>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <type_traits>
#include <utility>

template <typename T>
class HeapArray {
public:
    static_assert(std::is_trivially_destructible_v<T>, "T must be trivially destructible");

    using iterator = T*;
    using const_iterator = const T*;

    HeapArray() = default;

    explicit HeapArray(std::size_t count) :
        m_data{ sge::checked_alloc<T>(count) },
        m_count{ count }
    {}

    explicit HeapArray(std::size_t count, const T& value) : HeapArray(count) {
        std::fill_n(m_data, count, value);
    }

    HeapArray(const HeapArray& other) {
        const std::size_t size = other.m_count * sizeof(T);
        m_data = new T[size];
        m_count = other.m_count;
        std::memcpy(m_data, other.m_data, size);
    }

    HeapArray(HeapArray&& other) noexcept {
        operator=(std::move(other));
    }

    HeapArray& operator=(HeapArray&& other) noexcept {
        m_data = other.m_data;
        m_count = other.m_count;
        other.m_data = nullptr;
        other.m_count = 0;
        return *this;
    }

    [[nodiscard]]
    std::size_t count() const noexcept {
        return m_count;
    }

    [[nodiscard]]
    std::size_t size() const noexcept {
        return m_count * sizeof(T);
    }

    [[nodiscard]]
    T* data() noexcept {
        return m_data;    
    }

    [[nodiscard]]
    const T* data() const noexcept {
        return m_data;
    }

    iterator begin() noexcept {
        return m_data;
    }

    iterator end() noexcept {
        return m_data + m_count;
    }

    const_iterator cbegin() noexcept {
        return m_data;
    }

    const_iterator cend() noexcept {
        return m_data + m_count;
    }

    [[nodiscard]]
    inline T& operator[](std::size_t n) noexcept {
        return m_data[n];
    }

    [[nodiscard]]
    inline const T& operator[](std::size_t n) const noexcept {
        return m_data[n];
    }

    ~HeapArray() {
        if (m_data != nullptr) {
            free(m_data);
        }
        m_count = 0;
    }

private:
    T* m_data = nullptr;
    std::size_t m_count = 0;
};

#endif