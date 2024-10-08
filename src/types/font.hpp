#pragma once

#ifndef TERRARIA_TYPES_FONT
#define TERRARIA_TYPES_FONT

#include <unordered_map>
#include "texture.hpp"
#include "glyph.hpp"

struct Font {
    float font_size;
    Texture texture;
    std::unordered_map<char, Glyph> glyphs;
};

#endif