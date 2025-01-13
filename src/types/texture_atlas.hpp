#ifndef TYPES_TEXTURE_ATLAS_HPP
#define TYPES_TEXTURE_ATLAS_HPP

#pragma once

#include <utility>
#include <vector>

#include "../math/rect.hpp"
#include "../assert.hpp"

#include "texture.hpp"

struct TextureAtlas {
    TextureAtlas() = default;

    TextureAtlas(const Texture &texture, std::vector<math::Rect> rects, glm::uvec2 size, uint32_t columns, uint32_t rows) :
        m_rects(std::move(rects)),
        m_texture(texture),
        m_size(size),
        m_columns(columns),
        m_rows(rows) {}

    static TextureAtlas from_grid(const Texture& texture, const glm::uvec2& tile_size, uint32_t columns, uint32_t rows, const glm::uvec2& padding = glm::uvec2(0), const glm::uvec2& offset = glm::uvec2(0));

    [[nodiscard]] inline const std::vector<math::Rect>& rects() const { return m_rects; }
    [[nodiscard]] inline const Texture& texture() const { return m_texture; }
    [[nodiscard]] inline const glm::uvec2& size() const { return m_size; }
    [[nodiscard]] inline uint32_t columns() const { return m_columns; }
    [[nodiscard]] inline uint32_t rows() const { return m_rows; }
    [[nodiscard]] const math::Rect& get_rect(size_t index) const {
        ASSERT(index < m_rects.size(), "Index is out of bounds.");
        return m_rects[index];
    }

private:
    std::vector<math::Rect> m_rects;
    Texture m_texture;
    glm::uvec2 m_size;
    uint32_t m_columns;
    uint32_t m_rows;
};

#endif