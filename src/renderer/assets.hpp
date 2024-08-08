#ifndef TERRARIA_RENDERER_ASSETS
#define TERRARIA_RENDERER_ASSETS

#pragma once

#include <LLGL/Utils/VertexFormat.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct SpriteVertex {
    glm::vec3 position;
    glm::quat rotation;
    glm::vec2 size;
    glm::vec2 offset;
    glm::vec4 uv_offset_scale;
    glm::vec4 color;
    glm::vec4 outline_color;
    float outline_thickness;
    int has_texture;
    int is_ui;
    int is_nonscalable;
};

const LLGL::VertexFormat& SpriteVertexFormat();
const LLGL::VertexFormat& TilemapVertexFormat();
const LLGL::VertexFormat& GlyphVertexFormat();
const LLGL::VertexFormat& BackgroundVertexFormat();

#endif