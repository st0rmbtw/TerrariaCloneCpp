#version 330 core

out vec4 frag_color;

uniform sampler2DArray u_texture_array;

in vec2 v_uv;
flat in uint v_tile_id;

void main() {
    vec4 color = texture(u_texture_array, vec3(v_uv, float(v_tile_id)));
    if (color.a < 0.5) discard;
    
    frag_color = color;
}