#pragma once

#ifndef TERRARIA_RENDERER_ASSETS
#define TERRARIA_RENDERER_ASSETS

#include <LLGL/LLGL.h>
#include <LLGL/Utils/VertexFormat.h>
#include <glm/glm.hpp>

struct SpriteVertex {
    glm::vec4 transform_col_0;
    glm::vec4 transform_col_1;
    glm::vec4 transform_col_2;
    glm::vec4 transform_col_3;
    glm::vec4 uv_offset_scale;
    glm::vec4 color;
};

LLGL::VertexFormat SpriteVertexFormat(void);

#endif