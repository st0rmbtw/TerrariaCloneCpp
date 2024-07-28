#pragma once

#ifndef TERRARIA_RENDERER_WORLD_RENDERER
#define TERRARIA_RENDERER_WORLD_RENDERER

#include <LLGL/LLGL.h>
#include "world/world.hpp"

class WorldRenderer {
public:
    void Init();
    void Render(const World& world);
    void Terminate();

    inline void set_projection_matrix(const glm::mat4& projection) { m_projection_matrix = projection; }
    inline void set_view_matrix(const glm::mat4& view) { m_view_matrix = view; }

private:
    glm::mat4 m_projection_matrix;
    glm::mat4 m_view_matrix;

    LLGL::Buffer* m_constant_buffer = nullptr;
    LLGL::Buffer* m_transform_buffer = nullptr;
    LLGL::PipelineState* m_pipeline = nullptr;
};

#endif