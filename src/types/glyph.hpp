#pragma once

#ifndef TERRARIA_TYPES_GLYPH
#define TERRARIA_TYPES_GLYPH

#include <glm/vec2.hpp>

struct Glyph {
    glm::ivec2 size;
    glm::vec2 tex_size;
    glm::ivec2 bearing;
    signed long advance;
    glm::vec2 texture_coords;
};

#endif