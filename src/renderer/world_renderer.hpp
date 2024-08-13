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

    inline void set_depth(uint32_t wall_depth, uint32_t tile_depth) {
        m_wall_depth = wall_depth;
        m_tile_depth = tile_depth;
    }

private:
    LLGL::Buffer* m_depth_buffer = nullptr;
    LLGL::PipelineState* m_pipeline = nullptr;
    uint32_t m_tile_depth;
    uint32_t m_wall_depth;
};

#endif