#ifndef TERRARIA_TEXTURE_HPP
#define TERRARIA_TEXTURE_HPP

#pragma once

#include <LLGL/Texture.h>
#include <glm/glm.hpp>
#include "../common.h"

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
        ASSERT(texture != nullptr, "texture is null");
        const LLGL::Extent3D extent = texture->GetDesc().extent;
        return {extent.width, extent.height};
    }
};

#endif