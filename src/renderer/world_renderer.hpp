#pragma once

#ifndef TERRARIA_RENDERER_WORLD_RENDERER
#define TERRARIA_RENDERER_WORLD_RENDERER

#include <LLGL/LLGL.h>
#include "../world/world.hpp"

class WorldRenderer {
public:
    WorldRenderer() = default;

    void init();
    void render(const World& world);
    void terminate();

private:
    LLGL::Buffer* m_transform_buffer = nullptr;
    LLGL::PipelineState* m_pipeline = nullptr;
};

#endif