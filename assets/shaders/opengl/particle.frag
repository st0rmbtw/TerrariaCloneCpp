#version 330 core

layout(location = 0) out vec4 frag_color;

in vec2 g_uv;

uniform sampler2D u_texture;

void main() {
    vec4 color = texture(u_texture, g_uv);

    if (color.a < 0.5) discard;

    frag_color = color;
}