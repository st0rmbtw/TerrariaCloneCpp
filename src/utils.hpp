#pragma once

#ifndef UTILS_HPP_
#define UTILS_HPP_

#include <cstdlib>
#include <list>
#include <vector>
#include <optional>
#include <format>

#include <SGE/profile.hpp>

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
inline auto map(const std::optional<T>& a, F&& func) noexcept -> std::optional<decltype(func(a.value()))> {
    if (!a.has_value()) return std::nullopt;
    return std::forward<F>(func)(a.value());
}

template <typename TNode, typename TGetNeighborsFunc, typename TProcessNodeFunc>
inline void dfs(TNode start_node, TGetNeighborsFunc&& get_neighbors_func, TProcessNodeFunc&& process_node_func) {
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

template <class T>
inline void hash_combine(std::size_t& hash, const T& v) {
    hash ^= std::hash<T>{}(v) + 0x9e3779b9 + (hash<<6) + (hash>>2);
}

template <typename... _Args>
std::string_view temp_format(std::format_string<_Args...> __fmt, _Args&&... __args) {
    static std::string buffer;

    buffer.clear();
    
    std::format_to(
        std::back_inserter(buffer),
        __fmt,
        std::forward<_Args>(__args)...
    );

    return buffer;
}

#endif