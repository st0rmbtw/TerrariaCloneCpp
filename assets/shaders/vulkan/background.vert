#version 450 core

layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec2 a_size;
layout(location = 3) in vec2 a_tex_size;
layout(location = 4) in vec2 a_speed;
layout(location = 5) in int a_nonscale;

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
layout(location = 1) flat out vec2 v_size;
layout(location = 2) flat out vec2 v_tex_size;
layout(location = 3) flat out vec2 v_speed;
layout(location = 4) flat out vec2 v_offset;
layout(location = 5) flat out int v_nonscale;

void main() {
    mat4 view_proj = a_nonscale > 0 ? global_ubo.nonscale_view_projection : global_ubo.view_projection;
    mat4 proj_model = global_ubo.nonscale_projection * global_ubo.transform_matrix;

    vec2 offset = (proj_model * vec4(global_ubo.camera_position.x, global_ubo.camera_position.y, 0.0, 1.0)).xy;

    gl_Position = view_proj * vec4(a_position, 0.0, 1.0);
    gl_Position.z = 0.0;
    v_uv = a_uv;
    v_size = a_size;
    v_tex_size = a_tex_size;
    v_speed = a_speed;
    v_offset = offset;
    v_nonscale = a_nonscale;
}