#pragma once

#ifndef TERRARIA_UTILS_HPP
#define TERRARIA_UTILS_HPP

#include <stdlib.h>
#include <GLFW/glfw3.h>
#include <LLGL/LLGL.h>

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

inline LLGL::SamplerDescriptor NearestSampler() {
    LLGL::SamplerDescriptor sampler_descriptor;
    sampler_descriptor.minFilter = LLGL::SamplerFilter::Nearest;
    sampler_descriptor.magFilter = LLGL::SamplerFilter::Nearest;
    sampler_descriptor.mipMapFilter = LLGL::SamplerFilter::Nearest;
    return sampler_descriptor;
}

inline LLGL::SamplerDescriptor LinearSampler() {
    LLGL::SamplerDescriptor sampler_descriptor;
    sampler_descriptor.minFilter = LLGL::SamplerFilter::Linear;
    sampler_descriptor.magFilter = LLGL::SamplerFilter::Linear;
    sampler_descriptor.mipMapFilter = LLGL::SamplerFilter::Linear;
    return sampler_descriptor;
}

#endif