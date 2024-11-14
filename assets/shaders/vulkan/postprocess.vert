#version 450 core

layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec2 a_world_size;

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

vec2 project_point2(mat4 mat, vec2 p) {
    vec4 res = mat[0] * p.x;
    res = mat[1] * p.y + res;
    res = mat[3] + res;
    return res.xy;
}

vec2 screen_to_world(vec2 ndc) {
    return project_point2(global_ubo.inv_view_proj, ndc);
}

layout(location = 0) out vec2 v_uv;
layout(location = 1) out vec2 v_light_uv;

void main() {
    vec2 world_pos = screen_to_world(a_position);
    vec2 light_uv = world_pos / a_world_size;

    v_uv = a_uv;
    v_light_uv = light_uv;
    gl_Position = vec4(a_position, 0, 1);
}