#pragma once

#ifndef _ENGINE_TYPES_FONT_HPP_
#define _ENGINE_TYPES_FONT_HPP_

#include <unordered_map>
#include "texture.hpp"
#include "glyph.hpp"

struct Font {
    std::unordered_map<uint32_t, Glyph> glyphs;
    Texture texture;
    float font_size;
    int16_t ascender;
};

#endif