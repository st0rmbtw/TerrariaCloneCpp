#ifndef UTILS_HPP
#define UTILS_HPP

#pragma once

#include <stdlib.h>
#include <GLFW/glfw3.h>
#include <LLGL/LLGL.h>
#include <glm/glm.hpp>
#include <list>

#include "types/tile_pos.hpp"

#define ARRAY_LEN(array) (sizeof(array)/sizeof(array[0]))

bool FileExists(const char *path);

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

static inline constexpr bool check_bitflag(uint8_t data, uint8_t flag) {
    return (data & flag) == flag;
}

static inline constexpr bool remove_bitflag(uint8_t data, uint8_t flag) {
    return data & ~flag;
}

#endif