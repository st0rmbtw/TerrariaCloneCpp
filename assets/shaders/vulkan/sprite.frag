#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 2) uniform texture2D u_texture;
layout(set = 0, binding = 3) uniform sampler u_sampler;

layout(location = 0) in vec2 v_uv;
layout(location = 1) flat in vec4 v_color;
layout(location = 2) flat in int has_texture;

layout(location = 0) out vec4 frag_color;

void main() {
    if (has_texture > 0) {
        frag_color = texture(sampler2D(u_texture, u_sampler), v_uv) * v_color;
    } else {
        frag_color = v_color;
    }
}