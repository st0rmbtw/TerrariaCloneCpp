#ifndef TERRARIA_UTILS_HPP
#define TERRARIA_UTILS_HPP

#pragma once

#include <stdlib.h>
#include <GLFW/glfw3.h>
#include <LLGL/LLGL.h>
#include <glm/glm.hpp>
#include <list>

#include "assets.hpp"

inline const char* glfwGetErrorString() {
    const char* description = nullptr;
    glfwGetError(&description);
    return description;
}

bool FileExists(const char *path);

inline int rand_range(int from, int to) {
    return rand() % (to + 1 - from) + from;
}

inline float rand_range(float from, float to) {
    const float scale = rand() / (float) RAND_MAX;
    return from + scale * (to - from); 
}

template <class T>
static const T& list_at(const std::list<T>& list, int index) {
    auto it = list.begin();
    for (int i = 0; i < index; i++){
        ++it;
    }
    return *it;
}

glm::vec2 calculate_text_bounds(FontKey key, const std::string &text, float size);

#endif