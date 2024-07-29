#ifndef TERRARIA_TEXTURE_HPP
#define TERRARIA_TEXTURE_HPP

#pragma once

#include <LLGL/LLGL.h>

namespace TextureSampler {
    enum : uint8_t {
        Linear = 0,
        Nearest = 1
    };
};

struct Texture {
    uint32_t id = -1;
    int sampler = 0;
    LLGL::Texture* texture = nullptr;

    [[nodiscard]] inline glm::uvec2 size() const {
        const LLGL::Extent3D extent = texture->GetDesc().extent;
        return {extent.width, extent.height};
    }
};

#endif