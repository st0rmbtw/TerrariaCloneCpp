#ifndef RENDERER_BACKGROUND_RENDERER
#define RENDERER_BACKGROUND_RENDERER

#pragma once

#include <LLGL/Buffer.h>
#include <LLGL/PipelineState.h>

#include "types.hpp"

#include "../types/background_layer.hpp"

#include "../engine/renderer/renderer.hpp"

struct LayerData {
    int offset;
};

class BackgroundRenderer {
public:
    void init();
    void render();
    void render_world();
    void terminate();

    inline void draw_layer(const BackgroundLayer& layer) {
        draw_layer_internal(layer, &m_buffer_ptr);
        m_layer_count++;
    }

    inline void draw_world_layer(const BackgroundLayer& layer) {
        draw_layer_internal(layer, &m_world_buffer_ptr);
        m_world_layer_count++;
    }

private:
    static void draw_layer_internal(const BackgroundLayer& layer, BackgroundInstance** p_buffer);

private:
    size_t m_layer_count = 0;
    size_t m_world_layer_count = 0;

    Renderer* m_renderer = nullptr;
    
    LLGL::PipelineState* m_pipeline = nullptr;
    LLGL::ResourceHeap* m_resource_heap = nullptr;
    LLGL::Buffer* m_vertex_buffer = nullptr;
    
    LLGL::Buffer* m_instance_buffer = nullptr;
    LLGL::Buffer* m_world_instance_buffer = nullptr;
    
    LLGL::BufferArray* m_buffer_array = nullptr;
    LLGL::BufferArray* m_world_buffer_array = nullptr;

    BackgroundInstance* m_buffer = nullptr;
    BackgroundInstance* m_buffer_ptr = nullptr;

    BackgroundInstance* m_world_buffer = nullptr;
    BackgroundInstance* m_world_buffer_ptr = nullptr;
};

#endif