#pragma once

#ifndef TERRARIA_UTILS_HPP
#define TERRARIA_UTILS_HPP

#include <stdlib.h>
#include <GLFW/glfw3.h>
#include <LLGL/LLGL.h>
#include <glm/glm.hpp>

inline const char* glfwGetErrorString(void) {
    const char* description;
    glfwGetError(&description);
    return description;
}

bool FileExists(const char *path);

inline int rand_range(int from, int to) {
    return rand() % (to + 1 - from) + from;
}

inline float rand_range(float from, float to) {
    float scale = rand() / (float) RAND_MAX;
    return from + scale * (to - from); 
}

inline LLGL::SamplerDescriptor SamplerNearest() {
    LLGL::SamplerDescriptor sampler_descriptor;
    sampler_descriptor.minFilter = LLGL::SamplerFilter::Nearest;
    sampler_descriptor.magFilter = LLGL::SamplerFilter::Nearest;
    sampler_descriptor.mipMapFilter = LLGL::SamplerFilter::Nearest;
    return sampler_descriptor;
}

inline LLGL::SamplerDescriptor SamplerLinear() {
    LLGL::SamplerDescriptor sampler_descriptor;
    sampler_descriptor.minFilter = LLGL::SamplerFilter::Linear;
    sampler_descriptor.magFilter = LLGL::SamplerFilter::Linear;
    sampler_descriptor.mipMapFilter = LLGL::SamplerFilter::Linear;
    return sampler_descriptor;
}

struct SamplerDescriptorEqual {
    bool operator()(const LLGL::SamplerDescriptor& a, const LLGL::SamplerDescriptor& b) const noexcept {
        return a.addressModeU == b.addressModeU &&
        a.addressModeV == b.addressModeV &&
        a.addressModeW == b.addressModeW &&
        a.borderColor[0] == b.borderColor[0] &&
        a.borderColor[1] == b.borderColor[1] &&
        a.borderColor[2] == b.borderColor[2] &&
        a.borderColor[3] == b.borderColor[3] &&
        a.compareEnabled == b.compareEnabled &&
        a.compareOp == b.compareOp &&
        a.debugName == b.debugName &&
        a.magFilter == b.magFilter &&
        a.maxAnisotropy == b.maxAnisotropy &&
        a.maxLOD == b.maxLOD &&
        a.minFilter == b.minFilter &&
        a.minLOD == b.minLOD &&
        a.mipMapEnabled == b.mipMapEnabled &&
        a.mipMapFilter == b.mipMapFilter &&
        a.mipMapLODBias == b.mipMapLODBias;
    }
};

unsigned long hash_str(const char *str);


struct SamplerDescriptorHasher {
    size_t operator()(const LLGL::SamplerDescriptor& k) const {
        size_t res = 17;
        res = res * 31 + std::hash<LLGL::SamplerAddressMode>()(k.addressModeU);
        res = res * 31 + std::hash<LLGL::SamplerAddressMode>()(k.addressModeV);
        res = res * 31 + std::hash<LLGL::SamplerAddressMode>()(k.addressModeW);
        for (uint32_t i = 0; i < 4; ++i) {
            res = res * 31 + std::hash<float>()(k.borderColor[i]);
        }
        res = res * 31 + std::hash<bool>()(k.compareEnabled);
        res = res * 31 + hash_str(k.debugName);
        res = res * 31 + std::hash<LLGL::SamplerFilter>()(k.magFilter);
        res = res * 31 + std::hash<LLGL::SamplerFilter>()(k.minFilter);
        res = res * 31 + std::hash<uint32_t>()(k.maxAnisotropy);
        res = res * 31 + std::hash<float>()(k.maxLOD);
        res = res * 31 + std::hash<uint32_t>()(k.minLOD);
        res = res * 31 + std::hash<bool>()(k.mipMapEnabled);
        res = res * 31 + std::hash<LLGL::SamplerFilter>()(k.mipMapFilter);
        res = res * 31 + std::hash<float>()(k.mipMapLODBias);
        return res;
    }
};

#endif