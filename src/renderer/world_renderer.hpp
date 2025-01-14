#pragma once

#ifndef RENDERER_WORLD_RENDERER_HPP
#define RENDERER_WORLD_RENDERER_HPP

#include <LLGL/LLGL.h>
#include "../world/world_data.hpp"
#include "../world/world.hpp"
#include "../world/chunk_manager.hpp"

class WorldRenderer {
public:
    WorldRenderer() = default;

    void init();
    void init_textures(const WorldData& world);

    void update_lightmap_texture(WorldData& world);

    void update_tile_texture(WorldData& world);

    void compute_light(const Camera& camera, const World& world);

    void render(const ChunkManager& chunk_manager);
    void terminate();

    inline LLGL::Texture* lightmap_texture() { return m_lightmap_texture; }

    inline LLGL::Texture* light_texture() { return m_light_texture; }
    inline LLGL::RenderTarget* light_texture_target() { return m_light_texture_target; }

private:
    LLGL::Buffer* m_depth_buffer = nullptr;
    LLGL::PipelineState* m_pipeline = nullptr;
    LLGL::ResourceHeap* m_resource_heap = nullptr;

    LLGL::Buffer* m_light_buffer = nullptr;
    LLGL::Texture* m_tile_texture = nullptr;
    LLGL::Texture* m_lightmap_texture = nullptr;
    LLGL::ResourceHeap* m_light_init_resource_heap = nullptr;
    LLGL::ResourceHeap* m_light_blur_resource_heap = nullptr;

    LLGL::Texture* m_light_texture = nullptr;
    LLGL::RenderTarget* m_light_texture_target = nullptr;

    LLGL::PipelineState* m_light_set_light_sources_pipeline = nullptr;
    LLGL::PipelineState* m_light_vertical_pipeline = nullptr;
    LLGL::PipelineState* m_light_horizontal_pipeline = nullptr;
};

#endif