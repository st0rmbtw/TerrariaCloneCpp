#ifndef UTILS_HPP
#define UTILS_HPP

#pragma once

#include <cstdlib>
#include <list>
#include <vector>
#include <unordered_set>
#include <optional>

#include <LLGL/LLGL.h>
#include <glm/glm.hpp>

#include "types/tile_pos.hpp"

#define BITFLAG_CHECK(_DATA, _FLAG) ((_DATA & _FLAG) == _FLAG)
#define BITFLAG_REMOVE(_DATA, _FLAG) (_DATA & ~_FLAG)

#define ARRAY_LEN(array) (sizeof(array)/sizeof(array[0]))

static inline int rand_int(int from, int to) {
    return from + rand() % (to + 1 - from);
}

static inline float rand_float(float from, float to) {
    const float scale = rand() / (float) RAND_MAX;
    return from + scale * (to - from);
}

static inline bool rand_bool(float probability) {
    if (probability >= 1.0f) return true;
    if (probability <= 0.0f) return false;

    return rand_float(0.0f, 1.0f) < probability;
}

static inline bool rand_bool() {
    return rand() & 1;
}

template <class T>
static const T& list_at(const std::list<T>& list, int index) {
    auto it = list.cbegin();
    for (int i = 0; i < index; i++){
        ++it;
    }
    return *it;
}

static inline TilePos get_lightmap_pos(glm::vec2 pos) {
    return glm::ivec2(pos * static_cast<float>(Constants::SUBDIVISION) / Constants::TILE_SIZE);
}

template <typename T, class F>
static inline auto map(std::optional<T> a, F&& func) -> std::optional<decltype(func(a.value()))> {
    if (!a.has_value()) return std::nullopt;
    return func(a.value());
}

template <typename TNode, typename TGetNeighborsFunc, typename TProcessNodeFunc>
static inline void bfs(TNode start_node, TGetNeighborsFunc&& get_neighbors_func, TProcessNodeFunc&& process_node_func) {
    std::vector<TNode> q;

    q.push_back(start_node);

    while (!q.empty()) {
        TNode current_node = q.back();
        q.pop_back();

        process_node_func(current_node);

        for (const TNode& neighbor : get_neighbors_func(current_node)) {
            q.push_back(neighbor);
        }
    }
}


#endif