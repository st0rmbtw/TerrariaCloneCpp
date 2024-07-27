#pragma once

#ifndef TERRARIA_UTILS_HPP
#define TERRARIA_UTILS_HPP

#include <stdlib.h>
#include <GLFW/glfw3.h>
#include <LLGL/LLGL.h>
#include <glm/glm.hpp>
#include <list>

inline const char* glfwGetErrorString(void) {
    const char* description;
    glfwGetError(&description);
    return description;
}

bool FileExists(const char *path);

inline int rand_range(int from, int to) {
    return rand() % (to + 1 - from) + from;
}

inline float rand_range(float from, float to) {
    float scale = rand() / (float) RAND_MAX;
    return from + scale * (to - from); 
}

template <class T>
static const T& list_at(const std::list<T>& list, int index) {
    typename std::list<T>::const_iterator it = list.begin();
    for (int i = 0; i < index; i++){
        ++it;
    }
    return *it;
}

#endif