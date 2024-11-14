#version 450 core

layout(location = 0) out vec4 frag_color;

layout(binding = 3) uniform texture2D u_texture;
layout(binding = 4) uniform sampler u_sampler;

layout(location = 0) in vec2 v_uv;
layout(location = 1) flat in vec4 v_color;
layout(location = 2) flat in vec4 v_outline_color;
layout(location = 3) flat in float v_outline_thickness;
layout(location = 4) flat in int v_has_texture;

void main() {
    vec4 color = v_color;

    if (v_has_texture > 0) {
        if (v_outline_thickness > 0.0) {
            float outline = texture(sampler2D(u_texture, u_sampler), v_uv + vec2(v_outline_thickness, 0.0)).a;
            outline += texture(sampler2D(u_texture, u_sampler), v_uv + vec2(-v_outline_thickness, 0.0)).a;
            outline += texture(sampler2D(u_texture, u_sampler), v_uv + vec2(0.0, v_outline_thickness)).a;
            outline += texture(sampler2D(u_texture, u_sampler), v_uv + vec2(0.0, -v_outline_thickness)).a;
            outline += texture(sampler2D(u_texture, u_sampler), v_uv + vec2(v_outline_thickness, -v_outline_thickness)).a;
            outline += texture(sampler2D(u_texture, u_sampler), v_uv + vec2(-v_outline_thickness, v_outline_thickness)).a;
            outline += texture(sampler2D(u_texture, u_sampler), v_uv + vec2(v_outline_thickness, v_outline_thickness)).a;
            outline += texture(sampler2D(u_texture, u_sampler), v_uv + vec2(-v_outline_thickness, -v_outline_thickness)).a;
            outline = min(outline, 1.0);
            vec4 c = texture(sampler2D(u_texture, u_sampler), v_uv);
            color = mix(c, v_outline_color, outline);
        } else {
            color = texture(sampler2D(u_texture, u_sampler), v_uv) * v_color;
        }
    }

    if (color.a < 0.5) discard;

    frag_color = color;
}