#pragma once

#ifndef TERRARIA_SAMPLER_HPP
#define TERRARIA_SAMPLER_HPP

#include <LLGL/LLGL.h>

struct Sampler {
    uint32_t id;
    LLGL::Sampler* sampler;
    LLGL::SamplerDescriptor desc;
};

#endif