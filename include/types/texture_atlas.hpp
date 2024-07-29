#ifndef TERRARIA_TEXTURE_ATLAS_HPP
#define TERRARIA_TEXTURE_ATLAS_HPP

#pragma once

#include <utility>
#include <vector>
#include <LLGL/LLGL.h>
#include "math/rect.hpp"
#include "types/texture.hpp"

struct TextureAtlas {
    TextureAtlas() = default;

    TextureAtlas(const Texture &texture, std::vector<math::Rect> rects) :
        m_texture(texture),
        m_rects(std::move(rects)) {}

    static TextureAtlas from_grid(const Texture& texture, const glm::uvec2& tile_size, uint32_t columns, uint32_t rows, const glm::uvec2& padding = glm::uvec2(0), const glm::uvec2& offset = glm::uvec2(0));

    [[nodiscard]] const std::vector<math::Rect>& rects() const { return m_rects; }
    [[nodiscard]] const Texture& texture() const { return m_texture; }

private:
    Texture m_texture;
    std::vector<math::Rect> m_rects;
};

#endif