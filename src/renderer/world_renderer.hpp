#pragma once

#ifndef TERRARIA_RENDERER_WORLD_RENDERER
#define TERRARIA_RENDERER_WORLD_RENDERER

#include <LLGL/LLGL.h>
#include "../world/world_data.hpp"
#include "../world/chunk_manager.hpp"

class WorldRenderer {
public:
    WorldRenderer() = default;

    void init();
    void init_lightmap_texture(const WorldData& world);

    void update_lightmap_texture(WorldData& world, LightMapTaskResult lightmap);

    void render(const ChunkManager& chunk_manager);
    void terminate();

    inline LLGL::Texture* lightmap_texture() { return m_lightmap_texture; }

private:
    LLGL::Buffer* m_depth_buffer = nullptr;
    LLGL::PipelineState* m_pipeline = nullptr;
    LLGL::Texture* m_lightmap_texture = nullptr;
    LLGL::ResourceHeap* m_resource_heap = nullptr;
};

#endif