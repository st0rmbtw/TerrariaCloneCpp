#pragma once

#ifndef _ENGINE_RICH_TEXT_HPP_
#define _ENGINE_RICH_TEXT_HPP_

#include <string_view>
#include <utility>
#include <type_traits>
#include <array>
#include <glm/vec3.hpp>

struct RichTextSection {
    RichTextSection(std::string_view text, float size = 14.0f, glm::vec3 color = glm::vec3(1.0f)) :
        size(size),
        color(color),
        text(text) {}

    float size;
    glm::vec3 color;
    std::string_view text;
};

template <size_t Size, typename = void>
class RichText {
private:
    using sections_t = std::array<RichTextSection, 1>;
public:
    explicit inline RichText(std::string_view text, float size, glm::vec3 color) :
        m_sections{RichTextSection(text, size, color)} {}
    
    [[nodiscard]] inline const sections_t& sections() const { return m_sections; }
    [[nodiscard]] constexpr inline size_t size() const { return 1; }

private:
    sections_t m_sections;
};

template <size_t Size>
class RichText<Size, std::enable_if_t<(Size > 1)>> {
private:
    using sections_t = std::array<RichTextSection, Size>;
public:
    template<typename... N>
    constexpr inline RichText(N&&... args) : m_sections{std::forward<N>(args)...} {}
    
    [[nodiscard]] inline const sections_t& sections() const { return m_sections; }
    [[nodiscard]] constexpr inline size_t size() const { return Size; }

private:
    sections_t m_sections;
};

inline RichText<1> rich_text(std::string_view text, float size, glm::vec3 color) {
    return RichText<1>(text, size, color);
}

inline RichText<1> rich_text(const std::string& text, float size, glm::vec3 color) {
    return RichText<1>(std::string_view(text), size, color);
}

inline RichText<1> rich_text(std::string&& text, float size, glm::vec3 color) = delete;

inline RichText<1> rich_text(const char* text, float size, glm::vec3 color) {
    return RichText<1>(text, size, color);
}

template <typename... N>
constexpr inline auto rich_text(const N&&... args) -> RichText<sizeof...(args)> {
    return RichText<sizeof...(args)>(args...);
}

#endif