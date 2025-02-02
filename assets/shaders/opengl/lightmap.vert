#version 330 core

layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_uv;

layout(std140) uniform GlobalUniformBuffer {
    mat4 screen_projection;
    mat4 view_projection;
    mat4 nonscale_view_projection;
    mat4 nonscale_projection;
    mat4 transform_matrix;
    mat4 inv_view_proj;
    vec2 camera_position;
    vec2 window_size;
} global_ubo;

out vec2 v_uv;

void main() {
    v_uv = a_uv;

    gl_Position = global_ubo.view_projection * vec4(a_position, 0.0, 1.0);
    gl_Position.z = 1.0;
}