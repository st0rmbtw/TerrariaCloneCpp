#version 330 core

in vec2 g_uv;
flat in vec4 g_color;
flat in vec4 g_outline_color;
flat in float g_outline_thickness;
flat in int g_has_texture;

out vec4 frag_color;

uniform sampler2D u_texture;

void main() {
    if (g_has_texture > 0) {
        if (g_outline_thickness > 0.0) {
            float outline = texture(u_texture, g_uv + vec2(g_outline_thickness, 0.0)).a;
            outline += texture(u_texture, g_uv + vec2(-g_outline_thickness, 0.0)).a;
            outline += texture(u_texture, g_uv + vec2(0.0, g_outline_thickness)).a;
            outline += texture(u_texture, g_uv + vec2(0.0, -g_outline_thickness)).a;
            outline += texture(u_texture, g_uv + vec2(g_outline_thickness, -g_outline_thickness)).a;
            outline += texture(u_texture, g_uv + vec2(-g_outline_thickness, g_outline_thickness)).a;
            outline += texture(u_texture, g_uv + vec2(g_outline_thickness, g_outline_thickness)).a;
            outline += texture(u_texture, g_uv + vec2(-g_outline_thickness, -g_outline_thickness)).a;
            outline = min(outline, 1.0);
            vec4 c = texture(u_texture, g_uv);
            frag_color = mix(c, g_outline_color, outline);
        } else {
            frag_color = texture(u_texture, g_uv) * g_color;
        }
    } else {
        frag_color = g_color;
    }
}