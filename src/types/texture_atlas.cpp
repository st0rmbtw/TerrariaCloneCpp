#include "texture_atlas.hpp"

TextureAtlas TextureAtlas::from_grid(const Texture& texture, const glm::uvec2 &tile_size, uint32_t columns, uint32_t rows, const glm::uvec2& padding, const glm::uvec2& offset) {
    std::vector<math::Rect> sprites;

    glm::uvec2 current_padding = offset;

    for (size_t y = 0; y < rows; ++y) {
        if (y > 0) {
            current_padding.y = padding.y;
        }

        for (size_t x = 0; x < columns; ++x) {
            if (x > 0) {
                current_padding.x = padding.x;
            }

            const glm::uvec2 cell = glm::uvec2(x, y);

            const glm::uvec2 rect_min = (tile_size + current_padding) * cell + offset;

            sprites.emplace_back(rect_min, rect_min + tile_size);
        }
    }

    return TextureAtlas(texture, sprites, tile_size, columns, rows);
}