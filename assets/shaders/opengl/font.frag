#version 330 core

layout(location = 0) out vec4 frag_color;

in vec2 v_uv;
flat in vec3 v_color;

uniform sampler2D u_texture;

void main() {
    vec4 color = vec4(v_color, texture(u_texture, v_uv).r);

    if (color.a <= 0.05) discard;

    frag_color = color;
}