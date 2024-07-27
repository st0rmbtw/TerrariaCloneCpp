#version 330 core

in vec2 v_uv;
flat in vec4 v_color;
flat in int v_has_texture;

out vec4 frag_color;

uniform sampler2D u_texture;

void main() {
    if (v_has_texture > 0) {
        frag_color = texture(u_texture, v_uv) * v_color;
    } else {
        frag_color = v_color;
    }
}