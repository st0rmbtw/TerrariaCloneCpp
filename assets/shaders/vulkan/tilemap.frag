#version 450 core

#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 3) uniform texture2DArray u_texture_array;
layout(set = 0, binding = 4) uniform sampler u_sampler;

layout(location = 0) in vec2 g_uv;
layout(location = 1) flat in uint g_tile_id;

layout(location = 0) out vec4 frag_color;


void main() {
    vec4 color = texture(sampler2DArray(u_texture_array, u_sampler), vec3(g_uv, float(g_tile_id)));
    if (color.a < 0.5) discard;
    
    frag_color = color;
}