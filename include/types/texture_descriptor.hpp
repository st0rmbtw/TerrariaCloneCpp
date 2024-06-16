#pragma once

#ifndef TERRARIA_TEXTURE_DESCRIPTOR_HPP
#define TERRARIA_TEXTURE_DESCRIPTOR_HPP

#include <stdint.h>

enum class TextureWrapping : uint8_t {
    ClampToBorder = 0,
    ClampToEdge = 1,
    Repeat = 2,
};

enum class TextureSampler : uint8_t {
    Nearest = 0,
    Linear = 1
};

enum class TextureType : uint8_t {
    Texture2D = 0,
    Texture2DArray = 1,
};

struct TextureDescriptor {
    TextureSampler sampler = TextureSampler::Nearest;
    TextureWrapping wrapping = TextureWrapping::ClampToBorder;
};

#endif