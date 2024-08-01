#ifndef TERRARIA_TEXTURE_ATLAS_HPP
#define TERRARIA_TEXTURE_ATLAS_HPP

#pragma once

#include <utility>
#include <vector>
#include "../math/rect.hpp"
#include "../common.h"
#include "texture.hpp"

struct TextureAtlas {
    TextureAtlas() = default;

    TextureAtlas(const Texture &texture, std::vector<math::Rect> rects) :
        m_texture(texture),
        m_rects(std::move(rects)) {}

    static TextureAtlas from_grid(const Texture& texture, const glm::uvec2& tile_size, uint32_t columns, uint32_t rows, const glm::uvec2& padding = glm::uvec2(0), const glm::uvec2& offset = glm::uvec2(0));

    [[nodiscard]] inline const std::vector<math::Rect>& rects() const { return m_rects; }
    [[nodiscard]] inline const Texture& texture() const { return m_texture; }
    [[nodiscard]] const math::Rect& get_rect(size_t index) const {
        ASSERT(index < m_rects.size(), "Index is out of bounds.");
        return m_rects[index];
    }

private:
    Texture m_texture;
    std::vector<math::Rect> m_rects;
};

#endif