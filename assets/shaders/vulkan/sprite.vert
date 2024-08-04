#version 450 core

layout(location = 0) in vec4 a_transform_col_0;
layout(location = 1) in vec4 a_transform_col_1;
layout(location = 2) in vec4 a_transform_col_2;
layout(location = 3) in vec4 a_transform_col_3;
layout(location = 4) in vec4 a_uv_offset_scale;
layout(location = 5) in vec4 a_color;
layout(location = 6) in vec4 a_outline_color;
layout(location = 7) in float a_outline_thickness;
layout(location = 8) in int a_has_texture;
layout(location = 9) in int a_is_ui;

layout(location = 0) out VS_OUT {
    mat4 transform;
    vec4 uv_offset_scale;
    vec4 color;
    vec4 outline_color;
    float outline_thickness;
    int has_texture;
    int is_ui;
} vs_out;

void main() {
    mat4 transform = mat4(a_transform_col_0, a_transform_col_1, a_transform_col_2, a_transform_col_3);

    gl_Position = vec4(1.0);

    vs_out.transform = transform;
    vs_out.uv_offset_scale = a_uv_offset_scale;
    vs_out.color = a_color;
    vs_out.outline_color = a_outline_color;
    vs_out.outline_thickness = a_outline_thickness;
    vs_out.has_texture = a_has_texture;
    vs_out.is_ui = a_is_ui;
}