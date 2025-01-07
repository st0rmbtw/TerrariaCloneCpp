#pragma once

#ifndef TERRARIA_RENDERER_WORLD_RENDERER
#define TERRARIA_RENDERER_WORLD_RENDERER

#include <LLGL/LLGL.h>
#include "../world/world_data.hpp"
#include "../world/world.hpp"
#include "../world/chunk_manager.hpp"

class WorldRenderer {
public:
    WorldRenderer() = default;

    void init();
    void init_textures(const WorldData& world);

    void update_lightmap_texture(WorldData& world, LightMapTaskResult lightmap);

    void update_tile_texture(TilePos pos, uint8_t value);

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
    Light* m_light_buffer_data = nullptr;
    Light* m_light_buffer_data_ptr = nullptr;
    LLGL::Texture* m_tile_texture = nullptr;
    LLGL::Texture* m_lightmap_texture = nullptr;
    LLGL::ResourceHeap* m_light_resource_heap = nullptr;

    LLGL::Texture* m_light_texture = nullptr;
    LLGL::RenderTarget* m_light_texture_target = nullptr;

    LLGL::PipelineState* m_light_set_light_sources_pipeline = nullptr;
    LLGL::PipelineState* m_light_top_to_bottom_pipeline = nullptr;
    LLGL::PipelineState* m_light_bottom_to_top_pipeline = nullptr;
    LLGL::PipelineState* m_light_left_to_right_pipeline = nullptr;
    LLGL::PipelineState* m_light_right_to_left_pipeline = nullptr;


    bool m_light_resource_heap_initialized = false;
};

#endif