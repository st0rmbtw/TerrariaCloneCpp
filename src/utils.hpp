#ifndef UTILS_HPP
#define UTILS_HPP

#pragma once

#include <stdlib.h>
#include <GLFW/glfw3.h>
#include <LLGL/LLGL.h>
#include <glm/glm.hpp>
#include <list>

#include <optional>

#include "types/tile_pos.hpp"

#define BITFLAG_CHECK(_DATA, _FLAG) ((_DATA & _FLAG) == _FLAG)
#define BITFLAG_REMOVE(_DATA, _FLAG) (_DATA & ~_FLAG)

#define ARRAY_LEN(array) (sizeof(array)/sizeof(array[0]))

static inline int rand_range(int from, int to) {
    return rand() % (to + 1 - from) + from;
}

static inline float rand_range(float from, float to) {
    const float scale = rand() / (float) RAND_MAX;
    return from + scale * (to - from); 
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

#endif