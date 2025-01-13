#ifndef UTILS_HPP
#define UTILS_HPP

#pragma once

#include <stdlib.h>
#include <GLFW/glfw3.h>
#include <LLGL/LLGL.h>
#include <glm/glm.hpp>
#include <list>

#include "types/rich_text.hpp"
#include "types/tile_pos.hpp"

#include "assets.hpp"

#define ARRAY_LEN(array) (sizeof(array)/sizeof(array[0]))

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

uint32_t next_utf8_codepoint(const char** p_text);
glm::vec2 calculate_text_bounds(const char* text, float size, FontAsset key);

inline glm::vec2 calculate_text_bounds(const std::string& text, float size, FontAsset key) {
    return calculate_text_bounds(text.c_str(), size, key);
}

inline glm::vec2 calculate_text_bounds(const RichText<1>& text, FontAsset key) {
    const RichTextSection& section = text.sections()[0];
    return calculate_text_bounds(section.text, section.size, key);
}

template <size_t Size>
inline glm::vec2 calculate_text_bounds(const RichText<Size>& text, FontAsset key) {
    glm::vec2 bounds = glm::vec2(0.0f);

    for (size_t i = 0; i < Size; ++i) {
        const RichTextSection& section = text.sections()[i];
        const glm::vec2 section_bounds = calculate_text_bounds(section.text, section.size, key);
        bounds.x = std::max(section_bounds.x, bounds.x);
        bounds.y = std::max(section_bounds.y, bounds.y);
    }

    return bounds;
}

inline TilePos get_lightmap_pos(glm::vec2 pos) {
    return glm::ivec2(pos * static_cast<float>(Constants::SUBDIVISION) / Constants::TILE_SIZE);
}

#endif