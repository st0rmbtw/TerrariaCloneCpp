#pragma once

#ifndef TERRARIA_TEXTURE_HPP
#define TERRARIA_TEXTURE_HPP

#include <LLGL/LLGL.h>

struct Texture {
    uint32_t id;
    LLGL::Texture* texture;
    LLGL::SamplerDescriptor sampler;
};

#endif