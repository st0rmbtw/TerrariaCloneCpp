#version 450 core

layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_uv;

layout(binding = 2) uniform GlobalUniformBuffer {
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

layout(location = 0) out vec2 v_uv;

void main() {
    v_uv = a_uv;

    gl_Position = global_ubo.view_projection * vec4(a_position, 0.0, 1.0);
    gl_Position.z = 1.0;
}