#pragma once

#ifndef TERRARIA_RENDERER_WORLD_RENDERER
#define TERRARIA_RENDERER_WORLD_RENDERER

#include <LLGL/LLGL.h>
#include "../world/world.hpp"

class WorldRenderer {
public:
    WorldRenderer() = default;

    void init();
    void init_lightmap_texture(const WorldData& world);

    void render(const World& world);
    void terminate();

    inline LLGL::Texture* lightmap_texture() { return m_lightmap_texture; }

private:
    LLGL::Buffer* m_depth_buffer = nullptr;
    LLGL::PipelineState* m_pipeline = nullptr;
    LLGL::Texture* m_lightmap_texture = nullptr;
};

#endif