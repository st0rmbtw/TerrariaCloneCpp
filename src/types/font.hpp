#pragma once

#ifndef TERRARIA_TYPES_FONT
#define TERRARIA_TYPES_FONT

#include <map>
#include "texture.hpp"
#include "glyph.hpp"

struct Font {
    Texture texture;
    float font_size;
    std::map<char, Glyph> glyphs;
};

#endif