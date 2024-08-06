#version 330 core

layout(location = 0) out vec4 frag_color;

in vec2 g_uv;
flat in uint g_tile_id;

uniform sampler2DArray u_texture_array;

void main() {
    vec4 color = texture(u_texture_array, vec3(g_uv, float(g_tile_id)));
    if (color.a < 0.5) discard;
    
    frag_color = color;
}