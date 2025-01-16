#ifndef UI_UTILS_HPP
#define UI_UTILS_HPP

#pragma once

#include "../types/rich_text.hpp"
#include "../assets.hpp"

uint32_t next_utf8_codepoint(const char* text, size_t& index);
glm::vec2 calculate_text_bounds(const char* text, size_t length, float size, FontAsset key);

inline glm::vec2 calculate_text_bounds(std::string_view text, float size, FontAsset key) {
    return calculate_text_bounds(text.data(), text.size(), size, key);
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

#endif