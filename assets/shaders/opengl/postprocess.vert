#version 330 core

layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec2 a_world_size;

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

vec2 project_point2(mat4 mat, vec2 p) {
    vec2 res = mat[0].xy * p.x;
    res = mat[1].xy * p.y + res;
    res = mat[3].xy + res;
    return res;
}

vec2 screen_to_world(vec2 ndc) {
    return project_point2(global_ubo.inv_view_proj, ndc);
}

out vec2 v_uv;
out vec2 v_light_uv;

void main() {
    vec2 world_pos = screen_to_world(a_position);
    vec2 light_uv = world_pos / a_world_size;

    v_uv = a_uv;
    v_light_uv = light_uv;
    gl_Position = vec4(a_position, 0, 1);
}