#include "utils.hpp"

#include <SGE/defines.hpp>

#if SGE_PLATFORM_APPLE
    #include <LLGL/Backend/Metal/NativeHandle.h>
#endif

bool SupportsAcceleratedDynamicLighting(const sge::RenderContext& context) {
    const LLGL::RenderingFeatures& features = context.GetRenderingCaps().features;

#if SGE_PLATFORM_APPLE
    bool supportsReadWriteTextures = true;

    if (renderer.Backend().IsMetal()) {
        LLGL::Metal::RenderSystemNativeHandle metalNativeHandle;
        renderer.Context()->GetNativeHandle(&metalNativeHandle, sizeof(metalNativeHandle));
        supportsReadWriteTextures = metalNativeHandle.device.readWriteTextureSupport == MTLReadWriteTextureTier::MTLReadWriteTextureTier2;
    }

    return features.hasComputeShaders && supportsReadWriteTextures;
#else
    return features.hasComputeShaders;
#endif
}