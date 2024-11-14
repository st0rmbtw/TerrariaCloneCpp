#version 450 core

layout(location = 0) out vec4 frag_color;

layout(set = 0, binding = 3) uniform texture2D u_texture;
layout(set = 0, binding = 4) uniform sampler u_sampler;

layout(location = 0) in vec2 v_uv;

void main() {
    vec4 color = texture(sampler2D(u_texture, u_sampler), v_uv);

    if (color.a < 0.5) discard;

    frag_color = color;
}