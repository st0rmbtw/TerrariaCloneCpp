
#ifndef TERRARIA_RENDERER_BACKGROUND_RENDERER
#define TERRARIA_RENDERER_BACKGROUND_RENDERER

#pragma once

#include <LLGL/Buffer.h>
#include <LLGL/PipelineState.h>

#include "../types/background_layer.hpp"

struct BackgroundVertex {
    glm::vec2 position;
    glm::vec2 uv;
    glm::vec2 size;
    glm::vec2 tex_size;
    glm::vec2 speed;
    int nonscale;
};

struct LayerData {
    Texture texture;
    int offset;
};

class BackgroundRenderer {
public:
    void init();
    void render();
    void terminate();

    void draw_layer(const BackgroundLayer& layer);

private:
    std::vector<LayerData> m_layers;
    
    LLGL::PipelineState* m_pipeline = nullptr;
    LLGL::Buffer* m_vertex_buffer = nullptr;
    LLGL::Buffer* m_index_buffer = nullptr;
    BackgroundVertex* m_buffer = nullptr;
    BackgroundVertex* m_buffer_ptr = nullptr;
};

#endif