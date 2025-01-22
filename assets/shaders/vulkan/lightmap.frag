#version 450 core

layout(location = 0) out vec4 frag_color;

layout(binding = 3) uniform texture2D u_texture;
layout(binding = 4) uniform sampler u_sampler;

layout(location = 0) in vec2 v_uv;

void main() {
    frag_color = texture(sampler2D(u_texture, u_sampler), v_uv);
}