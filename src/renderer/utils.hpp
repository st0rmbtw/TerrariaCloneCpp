#ifndef _RENDERER_UTILS_HPP_
#define _RENDERER_UTILS_HPP_

#include <SGE/renderer/renderer.hpp>
#include <SGE/defines.hpp>

#if SGE_PLATFORM_APPLE
    #include <LLGL/Backend/Metal/NativeHandle.h>
#endif

bool SupportsAcceleratedDynamicLighting(const sge::Renderer& renderer);

#endif