#version 330 core

layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec2 a_size;
layout(location = 3) in vec2 a_tex_size;
layout(location = 4) in vec2 a_speed;
layout(location = 5) in int a_nonscale;

uniform GlobalUniformBuffer {
    mat4 screen_projection;
    mat4 view_projection;
    mat4 nonscale_view_projection;
    mat4 nonscale_projection;
    mat4 transform_matrix;
    vec2 camera_position;
    vec2 window_size;
} ubo;

out vec2 v_uv;
flat out vec2 v_size;
flat out vec2 v_tex_size;
flat out vec2 v_speed;
flat out int v_nonscale;

void main() {
    mat4 mvp = a_nonscale > 0 ? ubo.nonscale_view_projection : ubo.view_projection;

    gl_Position = mvp * vec4(a_position, 0.0, 1.0);
    gl_Position.z = 0.0;
    v_uv = a_uv;
    v_size = a_size;
    v_tex_size = a_tex_size;
    v_speed = a_speed;
    v_nonscale = a_nonscale;
}