#ifndef TERRARIA_TEXTURE_HPP
#define TERRARIA_TEXTURE_HPP

#include "LLGL/ResourceHeapFlags.h"
#pragma once

#include <LLGL/Texture.h>
#include <glm/glm.hpp>

namespace TextureSampler {
    enum : uint8_t {
        Linear = 0,
        LinearMips,
        Nearest,
        NearestMips,
    };
};

class Texture {
public:
    Texture() = default;

    Texture(uint32_t id, int sampler, glm::uvec2 size, LLGL::Texture* internal) :
        m_id(id),
        m_sampler(sampler),
        m_size(size),
        m_internal(internal) {}

    [[nodiscard]] inline uint32_t id() const { return m_id; }
    [[nodiscard]] inline int sampler() const { return m_sampler; }
    [[nodiscard]] inline glm::uvec2 size() const { return m_size; }
    [[nodiscard]] inline uint32_t width() const { return m_size.x; }
    [[nodiscard]] inline uint32_t height() const { return m_size.y; }

    inline operator LLGL::Resource*() const { return m_internal; }
    inline operator LLGL::Texture*() const { return m_internal; }
    inline operator LLGL::Texture&() const { return *m_internal; }
    inline operator LLGL::ResourceViewDescriptor() const { return m_internal; }

private:
    uint32_t m_id = -1;
    int m_sampler = 0;
    glm::uvec2 m_size = glm::uvec2(0, 0);
    LLGL::Texture* m_internal = nullptr;
};

#endif