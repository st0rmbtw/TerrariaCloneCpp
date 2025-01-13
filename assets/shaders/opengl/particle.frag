#version 330 core

layout(location = 0) out vec4 frag_color;

uniform sampler2D u_texture;

in vec2 v_uv;

void main() {
    vec4 color = texture(u_texture, v_uv);

    if (color.a < 0.5) discard;

    frag_color = color;
}