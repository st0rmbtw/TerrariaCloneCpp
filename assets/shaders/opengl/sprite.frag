#version 430 core

in vec2 v_uv;
flat in vec4 v_color;
flat in int v_texture_index;

out vec4 frag_color;

uniform sampler2D u_textures[gl_MaxTextureImageUnits];

void main() {
    if (v_texture_index >= 0) {
        frag_color = texture(u_textures[v_texture_index], v_uv);
    } else {
        frag_color = v_color;
    }
}