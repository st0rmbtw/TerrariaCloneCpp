#version 450 core
#extension GL_ARB_separate_shader_objects : enable

// layout(set = 0, binding = 1) uniform texture2D u_textures[32];
// layout(set = 0, binding = 2) uniform sampler u_samplers[32];

layout(location = 0) in vec2 v_uv;
layout(location = 1) flat in vec4 v_color;
layout(location = 2) flat in int v_texture_index;

layout(location = 0) out vec4 frag_color;

void main() {
    // if (v_texture_index >= 0) {
    //     frag_color = texture(sampler2D(u_textures[v_texture_index], u_samplers[v_texture_index]), v_uv);
    // } else {
        frag_color = v_color;
    // }
}