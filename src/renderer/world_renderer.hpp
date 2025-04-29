#pragma once

#ifndef RENDERER_WORLD_RENDERER_HPP
#define RENDERER_WORLD_RENDERER_HPP

#include <LLGL/LLGL.h>

#include <SGE/renderer/renderer.hpp>

#include "../world/world_data.hpp"
#include "../world/world.hpp"
#include "../world/chunk_manager.hpp"

#include "dynamic_lighting.hpp"

class WorldRenderer {
public:
    WorldRenderer() = default;

    void init();
    void init_lighting(const WorldData& world);
    void init_textures(const WorldData& world);
    void init_lightmap_chunks(const WorldData& world);
    
    void update(WorldData& world);

    void compute_light(const sge::Camera& camera, const World& world);

    void render(const ChunkManager& chunk_manager);
    void render_lightmap(const sge::Camera& camera);
    void terminate();

    void init_targets(LLGL::Extent2D resolution);

    inline LLGL::RenderTarget* target() { return m_target; }
    inline LLGL::Texture* target_texture() { return m_target_texture; }

    inline LLGL::RenderTarget* static_lightmap_target() { return m_static_lightmap_target; }
    inline LLGL::Texture* static_lightmap_texture() { return m_static_lightmap_texture; }

    [[nodiscard]]
    inline const LLGL::RenderPass* render_pass() const { return m_render_pass; }

    inline LLGL::Texture* light_texture() { return m_light_texture; }
    inline LLGL::RenderTarget* light_texture_target() { return m_light_texture_target; }
private:
    void update_lightmap_texture(WorldData& world);
private:
    std::unordered_map<glm::uvec2, LightMapChunk> m_lightmap_chunks;

    sge::Renderer* m_renderer = nullptr;

    LLGL::Buffer* m_tile_texture_data_buffer = nullptr;
    LLGL::PipelineState* m_pipeline = nullptr;
    LLGL::ResourceHeap* m_resource_heap = nullptr;

    LLGL::ResourceHeap* m_lightmap_resource_heap = nullptr;

    LLGL::Texture* m_light_texture = nullptr;
    LLGL::RenderTarget* m_light_texture_target = nullptr;

    LLGL::Texture* m_static_lightmap_texture = nullptr;
    LLGL::RenderTarget* m_static_lightmap_target = nullptr;
    LLGL::RenderPass* m_static_lightmap_render_pass = nullptr;

    LLGL::Texture* m_target_texture = nullptr;
    LLGL::Texture* m_depth_texture = nullptr;
    LLGL::RenderTarget* m_target = nullptr;
    LLGL::RenderPass* m_render_pass = nullptr;

    LLGL::PipelineState* m_lightmap_pipeline = nullptr;

    uint32_t m_lightmap_width = 0;
    uint32_t m_lightmap_height = 0;

    IDynamicLighting* m_dynamic_lighting = nullptr;
};

#endif