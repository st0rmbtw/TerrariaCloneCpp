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

    inline LLGL::RenderTarget* target() { return m_target.get(); }
    inline LLGL::Texture* target_texture() { return m_target_texture.get(); }

    inline LLGL::RenderTarget* static_lightmap_target() { return m_static_lightmap_target.get(); }
    inline LLGL::Texture* static_lightmap_texture() { return m_static_lightmap_texture.get(); }

    [[nodiscard]]
    inline const LLGL::RenderPass* render_pass() const { return m_render_pass.get(); }

    inline LLGL::Texture* light_texture() { return m_dynamic_light_texture.get(); }
    inline LLGL::RenderTarget* light_texture_target() { return m_dynamic_light_texture_target.get(); }
    
private:
    std::shared_ptr<sge::Renderer> m_renderer = nullptr;

    sge::LLGLResource<LLGL::Buffer> m_tile_texture_data_buffer = nullptr;
    sge::LLGLResource<LLGL::ResourceHeap> m_resource_heap = nullptr;
    
    sge::LLGLResource<LLGL::ResourceHeap> m_lightmap_resource_heap = nullptr;
    
    sge::LLGLResource<LLGL::Texture> m_dynamic_light_texture = nullptr;
    sge::LLGLResource<LLGL::RenderTarget> m_dynamic_light_texture_target = nullptr;
    
    sge::LLGLResource<LLGL::Texture> m_static_lightmap_texture = nullptr;
    sge::LLGLResource<LLGL::RenderTarget> m_static_lightmap_target = nullptr;
    sge::LLGLResource<LLGL::RenderPass> m_static_lightmap_render_pass = nullptr;
    
    sge::LLGLResource<LLGL::Texture> m_target_texture = nullptr;
    sge::LLGLResource<LLGL::Texture> m_depth_texture = nullptr;
    sge::LLGLResource<LLGL::RenderTarget> m_target = nullptr;
    sge::LLGLResource<LLGL::RenderPass> m_render_pass = nullptr;
    
    uint32_t m_pipeline_id = -1;
    uint32_t m_lightmap_pipeline_id = -1;

    std::unique_ptr<IDynamicLighting> m_dynamic_lighting = nullptr;
};

#endif