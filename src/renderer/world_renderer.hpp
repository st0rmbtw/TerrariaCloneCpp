#pragma once

#ifndef TERRARIA_RENDERER_WORLD_RENDERER
#define TERRARIA_RENDERER_WORLD_RENDERER

#include <LLGL/LLGL.h>
#include "../world/chunk_manager.hpp"

class WorldRenderer {
public:
    WorldRenderer() = default;

    void init();

    void render(const ChunkManager& chunk_manager);
    void terminate();

private:
    LLGL::Buffer* m_depth_buffer = nullptr;
    LLGL::PipelineState* m_pipeline = nullptr;
};

#endif