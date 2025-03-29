#ifndef _ENGINE_UTILS_HPP_
#define _ENGINE_UTILS_HPP_

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <string_view>

#include <glm/vec2.hpp>

#include "types/rich_text.hpp"
#include "types/font.hpp"

#include "log.hpp"

template <typename T> 
T* checked_alloc(size_t count) {
    T* _ptr = (T*) malloc(count * sizeof(T));
    if (_ptr == nullptr && count > 0) {
        LOG_ERROR("Out of memory");
        abort();
    }
    return _ptr;
}

uint32_t next_utf8_codepoint(const char* text, size_t& index);

glm::vec2 calculate_text_bounds(const char* text, size_t length, float size, const Font& font);

inline glm::vec2 calculate_text_bounds(std::string_view text, float size, const Font& font) {
    return calculate_text_bounds(text.data(), text.size(), size, font);
}

inline glm::vec2 calculate_text_bounds(const RichText<1>& text, const Font& font) {
    const RichTextSection& section = text.sections()[0];
    return calculate_text_bounds(section.text, section.size, font);
}

template <size_t Size>
inline glm::vec2 calculate_text_bounds(const RichText<Size>& text, const Font& font) {
    glm::vec2 bounds = glm::vec2(0.0f);

    for (size_t i = 0; i < Size; ++i) {
        const RichTextSection& section = text.sections()[i];
        const glm::vec2 section_bounds = calculate_text_bounds(section.text, section.size, font);
        bounds.x = std::max(section_bounds.x, bounds.x);
        bounds.y = std::max(section_bounds.y, bounds.y);
    }

    return bounds;
}

#endif