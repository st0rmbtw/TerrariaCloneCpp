#version 330 core

in vec2 g_uv;
flat in vec4 g_color;
flat in int g_has_texture;

out vec4 frag_color;

uniform sampler2D u_texture;

void main() {
    if (g_has_texture > 0) {
        frag_color = texture(u_texture, g_uv) * g_color;
    } else {
        frag_color = g_color;
    }
}