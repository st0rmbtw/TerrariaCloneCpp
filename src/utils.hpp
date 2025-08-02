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

template <class T>
inline const T& list_at(const std::list<T>& list, int index) {
    auto it = list.cbegin();
    for (int i = 0; i < index; i++){
        ++it;
    }
    return *it;
}

inline TilePos get_lightmap_pos(glm::vec2 pos) noexcept {
    return glm::ivec2(pos * static_cast<float>(Constants::SUBDIVISION) / Constants::TILE_SIZE);
}

template <typename T, class F>
inline auto map(std::optional<T> a, F&& func) noexcept -> std::optional<decltype(func(a.value()))> {
    if (!a.has_value()) return std::nullopt;
    return func(a.value());
}

template <typename TNode, typename TGetNeighborsFunc, typename TProcessNodeFunc>
inline void bfs(TNode start_node, TGetNeighborsFunc&& get_neighbors_func, TProcessNodeFunc&& process_node_func) {
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