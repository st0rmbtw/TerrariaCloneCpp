#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 2) uniform texture2D u_texture;
layout(set = 0, binding = 3) uniform sampler u_sampler;

layout(location = 0) in vec2 g_uv;
layout(location = 1) flat in vec4 g_color;
layout(location = 2) flat in int g_has_texture;

layout(location = 0) out vec4 frag_color;

void main() {
    if (g_has_texture > 0) {
        frag_color = texture(sampler2D(u_texture, u_sampler), g_uv) * g_color;
    } else {
        frag_color = g_color;
    }
}