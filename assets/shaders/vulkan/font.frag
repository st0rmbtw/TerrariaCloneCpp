#version 450 core

layout(location = 0) out vec4 frag_color;

layout(location = 0) in vec2 g_uv;
layout(location = 1) flat in vec3 g_color;

layout(set = 0, binding = 2) uniform texture2D u_texture;
layout(set = 0, binding = 3) uniform sampler u_sampler;

void main() {
    frag_color = vec4(g_color, texture(sampler2D(u_texture, u_sampler), g_uv).r);
}