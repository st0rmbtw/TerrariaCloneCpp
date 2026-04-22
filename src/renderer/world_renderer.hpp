#pragma once

#ifndef RENDERER_WORLD_RENDERER_HPP_
#define RENDERER_WORLD_RENDERER_HPP_

#include <LLGL/LLGL.h>

#include <SGE/renderer/renderer.hpp>

#include "../world/world_data.hpp"
#include "../world/world.hpp"
#include "../world/chunk_manager.hpp"

#include "dynamic_lighting.hpp"

class WorldRenderer {
public:
    WorldRenderer(const std::shared_ptr<sge::Renderer>& renderer);
    ~WorldRenderer();

    void init_lighting(const WorldData& world);
    void init_textures(LLGL::Extent2D viewport);
    
    void update(World& world);

    void compute_light(const sge::Camera& camera, const World& world);

    void render(const ChunkManager& chunk_manager);
    void render_lightmap(const ChunkManager& chunk_manager);

    void init_targets(LLGL::Extent2D resolution);

    inline sge::Handle<LLGL::RenderTarget> target() { return m_target; }
    inline const sge::Ref<LLGL::Texture>& target_texture() { return m_target_texture; }

    inline sge::Handle<LLGL::RenderTarget> static_lightmap_target() { return m_static_lightmap_target; }
    inline const sge::Ref<LLGL::Texture>& static_lightmap_texture() { return m_static_lightmap_texture; }

    [[nodiscard]]
    inline sge::Handle<LLGL::RenderPass> render_pass() const { return m_render_pass; }

    inline const sge::Ref<LLGL::Texture>& light_texture() { return m_dynamic_light_texture; }
    inline sge::Handle<LLGL::RenderTarget> light_texture_target() { return m_dynamic_light_texture_target; }
    
private:
    std::shared_ptr<sge::Renderer> m_renderer = nullptr;

    sge::Unique<LLGL::Buffer> m_tile_texture_data_buffer = nullptr;
    sge::Unique<LLGL::ResourceHeap> m_resource_heap = nullptr;
    sge::Unique<LLGL::ResourceHeap> m_lightmap_resource_heap = nullptr;
    
    sge::Ref<LLGL::Texture> m_dynamic_light_texture = nullptr;
    sge::Ref<LLGL::Texture> m_target_texture = nullptr;
    sge::Ref<LLGL::Texture> m_depth_texture = nullptr;
    sge::Ref<LLGL::Texture> m_static_lightmap_texture = nullptr;
    
    std::unique_ptr<IDynamicLighting> m_dynamic_lighting = nullptr;

    sge::Handle<LLGL::RenderPass> m_static_lightmap_render_pass;
    sge::Handle<LLGL::RenderPass> m_render_pass;
    sge::Handle<LLGL::RenderTarget> m_target;
    sge::Handle<LLGL::RenderTarget> m_static_lightmap_target;
    sge::Handle<LLGL::RenderTarget> m_dynamic_light_texture_target;
    
    sge::Handle<LLGL::PipelineState> m_pipeline;
    sge::Handle<LLGL::PipelineState> m_lightmap_pipeline;
};

#endif