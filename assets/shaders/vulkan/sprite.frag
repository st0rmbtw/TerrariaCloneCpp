#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 2) uniform texture2D u_texture;
layout(set = 0, binding = 3) uniform sampler u_sampler;

layout(location = 0) in vec2 g_uv;
layout(location = 1) flat in vec4 g_color;
layout(location = 2) flat in vec4 g_outline_color;
layout(location = 3) flat in float g_outline_thickness;
layout(location = 4) flat in int g_has_texture;

layout(location = 0) out vec4 frag_color;

void main() {
    if (g_has_texture > 0) {
        if (g_outline_thickness > 0.0) {
            float outline = texture(sampler2D(u_texture, u_sampler), g_uv + vec2(g_outline_thickness, 0.0)).a;
            outline += texture(sampler2D(u_texture, u_sampler), g_uv + vec2(-g_outline_thickness, 0.0)).a;
            outline += texture(sampler2D(u_texture, u_sampler), g_uv + vec2(0.0, g_outline_thickness)).a;
            outline += texture(sampler2D(u_texture, u_sampler), g_uv + vec2(0.0, -g_outline_thickness)).a;
            outline += texture(sampler2D(u_texture, u_sampler), g_uv + vec2(g_outline_thickness, -g_outline_thickness)).a;
            outline += texture(sampler2D(u_texture, u_sampler), g_uv + vec2(-g_outline_thickness, g_outline_thickness)).a;
            outline += texture(sampler2D(u_texture, u_sampler), g_uv + vec2(g_outline_thickness, g_outline_thickness)).a;
            outline += texture(sampler2D(u_texture, u_sampler), g_uv + vec2(-g_outline_thickness, -g_outline_thickness)).a;
            outline = min(outline, 1.0);
            vec4 c = texture(sampler2D(u_texture, u_sampler), g_uv);
            frag_color = mix(c, g_outline_color, outline);
        } else {
            frag_color = texture(sampler2D(u_texture, u_sampler), g_uv) * g_color;
        }
    } else {
        frag_color = g_color;
    }
}