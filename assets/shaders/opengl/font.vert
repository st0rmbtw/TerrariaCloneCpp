#version 330 core

layout(location = 0) in vec2 a_position;
layout(location = 1) in vec3 i_color;
layout(location = 2) in vec3 i_position;
layout(location = 3) in vec2 i_size;
layout(location = 4) in vec2 i_tex_size;
layout(location = 5) in vec2 i_uv;
layout(location = 6) in int i_ui;

layout(std140) uniform GlobalUniformBuffer {
    mat4 screen_projection;
    mat4 view_projection;
    mat4 nonscale_view_projection;
    mat4 nonscale_projection;
    mat4 transform_matrix;
    mat4 inv_view_proj;
    vec2 camera_position;
    vec2 window_size;
    float max_depth;
    float max_world_depth;
} global_ubo;

out vec2 v_uv;
flat out vec3 v_color;

void main() {
    mat4 mvp = i_ui > 0 ? global_ubo.screen_projection : global_ubo.view_projection;

    vec2 position = i_position.xy + a_position * i_size;
    vec2 uv = i_uv + a_position * i_tex_size;

    v_uv = uv;
    v_color = i_color;
    gl_Position = mvp * vec4(position, 0.0, 1.0);
    gl_Position.z = i_position.z / global_ubo.max_depth;
}