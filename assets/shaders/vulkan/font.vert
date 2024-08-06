#version 450 core

layout(location = 0) in vec3 a_color;
layout(location = 1) in vec3 a_position;
layout(location = 2) in vec2 a_uv;
layout(location = 3) in int a_ui;

layout(binding = 1) uniform UniformBuffer {
    mat4 screen_projection;
    mat4 view_projection;
} ubo;

layout(location = 0) out vec2 v_uv;
layout(location = 1) flat out vec3 v_color;

void main() {
    mat4 mvp = a_ui > 0 ? ubo.screen_projection : ubo.view_projection;

    gl_Position = mvp * vec4(a_position.x, a_position.y, 0.0, 1.0);
    gl_Position.z = a_position.z;
    v_uv = a_uv;
    v_color = a_color;
}