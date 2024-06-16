#pragma once

#ifndef TERRARIA_TEXTURE_ATLAS_HPP
#define TERRARIA_TEXTURE_ATLAS_HPP

#include <vector>
#include <LLGL/LLGL.h>
#include "math/rect.hpp"

struct TextureAtlas {
    TextureAtlas() {}

    TextureAtlas(const LLGL::Texture* texture, std::vector<math::Rect> rects) :
        m_texture(texture),
        m_rects(rects) {}

    static TextureAtlas from_grid(const LLGL::Texture* texture, const glm::uvec2& tile_size, uint32_t columns, uint32_t rows, const glm::uvec2& padding = glm::uvec2(0), const glm::uvec2& offset = glm::uvec2(0));

    const std::vector<math::Rect>& rects(void) const { return m_rects; }
    const LLGL::Texture* texture(void) const { return m_texture; }

private:
    const LLGL::Texture* m_texture;
    std::vector<math::Rect> m_rects;
};

#endif