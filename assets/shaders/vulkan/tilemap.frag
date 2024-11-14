#version 450 core

layout(location = 0) out vec4 frag_color;

layout(binding = 4) uniform texture2DArray u_texture_array;
layout(binding = 5) uniform sampler u_sampler;

layout(location = 0) in vec2 v_uv;
layout(location = 1) flat in uint v_tile_id;

void main() {
    vec4 color = texture(sampler2DArray(u_texture_array, u_sampler), vec3(v_uv, float(v_tile_id)));
    if (color.a < 0.5) discard;
    
    frag_color = color;
}