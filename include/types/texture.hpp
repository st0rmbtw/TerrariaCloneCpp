#pragma once

#ifndef TERRARIA_TEXTURE_HPP
#define TERRARIA_TEXTURE_HPP

#include <LLGL/LLGL.h>

namespace TextureSampler {
    enum {
        Linear = 0,
        Nearest = 1
    };
};

struct Texture {
    uint32_t id;
    int sampler;
    LLGL::Texture* texture;

    inline glm::uvec2 size() const {
        LLGL::Extent3D extent = texture->GetDesc().extent;
        return glm::uvec2(extent.width, extent.height);
    }
};

#endif