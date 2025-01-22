#version 330 core

layout(location = 0) out vec4 frag_color;

uniform sampler2D u_texture;

in vec2 v_uv;

void main() {
    frag_color = texture(u_texture, v_uv);
}