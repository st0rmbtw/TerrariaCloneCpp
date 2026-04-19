#pragma once

#ifndef UTILS_HPP_
#define UTILS_HPP_

#include <cstdlib>
#include <list>
#include <vector>
#include <optional>
#include <format>
#include <ranges>
#include <variant>
#include <type_traits>

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

template<std::ranges::random_access_range Range>
requires std::ranges::sized_range<Range>
constexpr std::ranges::range_reference_t<Range>
get_random(Range&& range) {
    using Index = std::iter_difference_t<std::ranges::iterator_t<Range>>;
    const size_t size = std::ranges::size(std::forward<Range>(range));

    const Index index = rand() % size;

    return std::ranges::begin(std::forward<Range>(range))[index];
}

template <typename> struct tag {};

template <typename T, typename V>
struct variant_index_;

template <typename T, typename... Ts> 
struct variant_index_<T, std::variant<Ts...>>
    : std::integral_constant<size_t, std::variant<tag<Ts>...>(tag<T>()).index()> {};

template <typename Variant, typename T>
inline constexpr size_t variant_index = variant_index_<T, Variant>::value;

template<typename _Tp, typename... _Types>
inline constexpr _Tp& unsafe_get(std::variant<_Types...>& v) noexcept {
    return *std::get_if<variant_index_<_Tp, std::variant<_Types...>>::value>(&v);
}

template<typename _Tp, typename... _Types>
inline constexpr const _Tp& unsafe_get(const std::variant<_Types...>& v) noexcept {
    return *std::get_if<variant_index_<_Tp, std::variant<_Types...>>::value>(&v);
}

#endif