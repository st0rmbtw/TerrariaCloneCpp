#version 450 core

layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_inv_tex_size;
layout(location = 2) in vec2 a_tex_size;
layout(location = 3) in vec2 i_uv;
layout(location = 4) in float i_depth;
layout(location = 5) in uint i_id;
layout(location = 6) in uint i_is_world;

layout(binding = 2) uniform GlobalUniformBuffer {
    mat4 screen_projection;
    mat4 view_projection;
    mat4 nonscale_view_projection;
    mat4 nonscale_projection;
    mat4 transform_matrix;
    mat4 inv_view_proj;
    vec2 camera_position;
    vec2 window_size;
} global_ubo;

layout(binding = 5) buffer TransformsBuffer
{
   vec4 transforms[];
};

layout(location = 0) out vec2 v_uv;

void main() {
    mat4 mvp = mat4(
        transforms[i_id * 4 + 0],
        transforms[i_id * 4 + 1],
        transforms[i_id * 4 + 2],
        transforms[i_id * 4 + 3]
    );

    vec2 uv = i_uv / a_tex_size;

    v_uv = uv + a_position * a_inv_tex_size;

    bool is_world = i_is_world > 0;
    
    gl_Position = mvp * vec4(a_position, 0.0, 1.0);
    gl_Position.z = i_depth;
}