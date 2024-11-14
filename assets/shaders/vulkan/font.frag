#version 450 core

layout(location = 0) out vec4 frag_color;

layout(location = 0) in vec2 v_uv;
layout(location = 1) flat in vec3 v_color;

layout(set = 0, binding = 3) uniform texture2D u_texture;
layout(set = 0, binding = 4) uniform sampler u_sampler;

void main() {
    vec4 color = vec4(v_color, texture(sampler2D(u_texture, u_sampler), v_uv).r);

    if (color.a <= 0.05) discard;

    frag_color = color;
}