#pragma once

#ifndef RENDERER_BACKGROUND_RENDERER_HPP_
#define RENDERER_BACKGROUND_RENDERER_HPP_

#include <LLGL/Buffer.h>
#include <LLGL/PipelineState.h>
#include <LLGL/PipelineLayout.h>

#include <SGE/renderer/renderer.hpp>

#include "world_renderer.hpp"

#include "types.hpp"

#include "../types/background_layer.hpp"

struct LayerData {
    int offset;
};

class BackgroundRenderer {
public:
    BackgroundRenderer(const std::shared_ptr<sge::Renderer>& renderer);

    void init_world(WorldRenderer& renderer);
    void init_targets(LLGL::Extent2D resolution);
    void render();
    void render_world();
    void reset();

    inline void draw_layer(const BackgroundLayer& layer) {
        draw_layer_internal(layer, &m_buffer_ptr);
        m_layer_count++;
    }

    inline void draw_world_layer(const BackgroundLayer& layer) {
        draw_layer_internal(layer, &m_world_buffer_ptr);
        m_world_layer_count++;
    }

    [[nodiscard]]
    sge::Handle<LLGL::RenderTarget> target() { return m_background_render_target; }
    const sge::Ref<LLGL::Texture>& target_texture() { return m_background_render_texture; }

private:
    static void draw_layer_internal(const BackgroundLayer& layer, BackgroundInstance** p_buffer);

private:
    size_t m_layer_count = 0;
    size_t m_world_layer_count = 0;

    std::shared_ptr<sge::Renderer> m_renderer = nullptr;

    sge::Ref<LLGL::PipelineLayout> m_pipeline_layout = nullptr;
    sge::Ref<LLGL::ResourceHeap> m_resource_heap = nullptr;
    sge::Ref<LLGL::Buffer> m_vertex_buffer = nullptr;
    
    sge::Ref<LLGL::Buffer> m_instance_buffer = nullptr;
    sge::Ref<LLGL::Buffer> m_world_instance_buffer = nullptr;
    
    sge::Ref<LLGL::BufferArray> m_buffer_array = nullptr;
    sge::Ref<LLGL::BufferArray> m_world_buffer_array = nullptr;
    
    sge::Ref<LLGL::Texture> m_background_render_texture = nullptr;

    HeapArray<BackgroundInstance> m_buffer;
    HeapArray<BackgroundInstance> m_world_buffer;
    
    BackgroundInstance* m_buffer_ptr = nullptr;
    BackgroundInstance* m_world_buffer_ptr = nullptr;
    
    sge::Handle<LLGL::RenderTarget> m_background_render_target;
    sge::Handle<LLGL::PipelineState> m_pipeline_world;
    sge::Handle<LLGL::PipelineState> m_pipeline;
};

#endif