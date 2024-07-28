#pragma once

#ifndef TERRARIA_RENDERER_ASSETS
#define TERRARIA_RENDERER_ASSETS

#include <LLGL/LLGL.h>
#include <LLGL/Utils/VertexFormat.h>
#include <glm/glm.hpp>
#include "types/texture.hpp"

struct SpriteVertex {
    glm::vec4 transform_col_0;
    glm::vec4 transform_col_1;
    glm::vec4 transform_col_2;
    glm::vec4 transform_col_3;
    glm::vec4 uv_offset_scale;
    glm::vec4 color;
    int has_texture;
    int is_ui;
};

struct SpriteUniforms {
    glm::mat4 view_projection;
    glm::mat4 screen_projection;
};

struct TilemapUniforms {
    glm::mat4 projection;
    glm::mat4 view;
};

const LLGL::VertexFormat& SpriteVertexFormat(void);
const LLGL::VertexFormat& TilemapVertexFormat(void);

#endif