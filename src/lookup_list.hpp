#pragma once

#ifndef LOOKUP_LIST_
#define LOOKUP_LIST_

#include <vector>
#include <algorithm>
#include <glm/glm.hpp>

#include <LLGL/Container/DynamicArray.h>
#include <LLGL/Tags.h>

#include <SGE/profile.hpp>

template <typename T>
class LookupList {
private:
    struct Cell {
        size_t index;
        size_t cell_key;
        bool visited = false;
    };

public:
    using const_iterator = std::vector<T>::const_iterator;
    using iterator = std::vector<T>::iterator;

public:
    LookupList(glm::vec2 cell_size) : m_cell_size(cell_size) {
        m_items.reserve(10);
        m_spatial_lookup.reserve(10);
        m_start_indices.resize(10, LLGL::UninitializeTag{});
    };

    inline void add(T&& item) {
        m_items.push_back(std::move(item));
        if (m_start_indices.capacity() < m_items.size())
            m_start_indices.resize(size(), LLGL::UninitializeTag{});
    }

    inline void add(const T& item) {
        m_items.push_back(item);
        m_spatial_lookup.resize(size());
        m_start_indices.resize(size());
    }

    inline void remove(size_t index) {
        m_items.erase(const_iterator(&m_items[index]));
    }

    inline void remove(const_iterator iterator) {
        m_items.erase(iterator);
    }

    inline void reserve(size_t n) {
        m_items.reserve(n);
        m_spatial_lookup.reserve(n);
        m_start_indices.resize(n, LLGL::UninitializeTag{});
    }

    [[nodiscard]]
    inline size_t size() const noexcept {
        return m_items.size();
    }

    [[nodiscard]]
    const_iterator cbegin() const noexcept {
        return m_items.cbegin();
    }

    [[nodiscard]]
    const_iterator cend() const noexcept {
        return m_items.cend();
    }

    [[nodiscard]]
    const_iterator begin() const noexcept {
        return m_items.begin();
    }

    [[nodiscard]]
    const_iterator end() const noexcept {
        return m_items.end();
    }

    [[nodiscard]]
    iterator begin() noexcept {
        return m_items.begin();
    }

    [[nodiscard]]
    iterator end() noexcept {
        return m_items.end();
    }

    T& operator[](size_t index) {
        return m_items[index];
    }

    template <typename F>
    void for_each_neighbor(glm::vec2 position, F&& func) {
        ZoneScoped;

        if (m_items.empty()) return;

        static glm::ivec2 OFFSETS[] = {
            glm::ivec2(0, 0),

            glm::ivec2(-1, 0),
            glm::ivec2(1, 0),
            glm::ivec2(0, -1),
            glm::ivec2(0, 1),

            glm::ivec2(-1, -1),
            glm::ivec2(1, 1),
            glm::ivec2(1, -1),
            glm::ivec2(-1, 1),
        };

        const glm::ivec2 center = pos_to_cell_coord(position);

        for (const glm::ivec2& offset : OFFSETS) {
            size_t key = get_key_from_hash(hash_cell(center + offset));
            size_t start_index = m_start_indices[key];

            for (size_t i = start_index; i < m_spatial_lookup.size(); ++i) {
                if (m_spatial_lookup[i].cell_key != key) break;
                if (m_spatial_lookup[i].visited) continue;

                m_spatial_lookup[i].visited = true;

                size_t particle_index = m_spatial_lookup[i].index;
                std::forward<F>(func)(particle_index, m_items[particle_index]);
            }
        }
    }

    void update_lookup() {
        ZoneScoped;

        if (m_items.empty()) return;

        m_spatial_lookup.clear();

        for (size_t i = 0; i < size(); ++i) {
            const glm::ivec2 coord = pos_to_cell_coord(m_items[i].position());
            size_t cell_key = get_key_from_hash(hash_cell(coord));
            m_spatial_lookup.emplace_back(i, cell_key, false);
            m_start_indices[i] = SIZE_MAX;
        };

        std::sort(
            m_spatial_lookup.begin(),
            m_spatial_lookup.end(),
            [](const Cell& a, const Cell& b) {
                return a.cell_key < b.cell_key;
            }
        );

        for (size_t i = 0; i < size(); ++i) {
            size_t key = m_spatial_lookup[i].cell_key;
            size_t prev_key = i == 0 ? SIZE_MAX : m_spatial_lookup[i - 1].cell_key;
            if (key != prev_key) {
                m_start_indices[key] = i;
            }
        };
    }

private:

    [[nodiscard]]
    inline size_t get_key_from_hash(size_t hash) const {
        return hash % m_items.capacity();
    }

    inline glm::ivec2 pos_to_cell_coord(glm::vec2 position) {
        const int x = position.x / m_cell_size.x;
        const int y = position.y / m_cell_size.y;
        return glm::vec2{ x, y };
    }

    template <class V>
    inline void hash_combine(std::size_t& s, const V& v) {
        s ^= std::hash<V>{}(v) + 0x9e3779b9 + (s<< 6) + (s>> 2);
    }

    inline size_t hash_cell(glm::ivec2 pos) {
        size_t hash = 0;
        hash_combine(hash, pos.x);
        hash_combine(hash, pos.y);
        return hash;
    }

private:
    std::vector<Cell> m_spatial_lookup;
    LLGL::DynamicArray<size_t> m_start_indices;
    std::vector<T> m_items;

    glm::vec2 m_cell_size;
};

#endif