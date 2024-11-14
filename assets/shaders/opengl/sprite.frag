#version 330 core

in vec2 v_uv;
flat in vec4 v_color;
flat in vec4 v_outline_color;
flat in float v_outline_thickness;
flat in int v_has_texture;

out vec4 frag_color;

uniform sampler2D u_texture;

void main() {
    vec4 color = v_color;

    if (v_has_texture > 0) {
        if (v_outline_thickness > 0.0) {
            float outline = texture(u_texture, v_uv + vec2(v_outline_thickness, 0.0)).a;
            outline += texture(u_texture, v_uv + vec2(-v_outline_thickness, 0.0)).a;
            outline += texture(u_texture, v_uv + vec2(0.0, v_outline_thickness)).a;
            outline += texture(u_texture, v_uv + vec2(0.0, -v_outline_thickness)).a;
            outline += texture(u_texture, v_uv + vec2(v_outline_thickness, -v_outline_thickness)).a;
            outline += texture(u_texture, v_uv + vec2(-v_outline_thickness, v_outline_thickness)).a;
            outline += texture(u_texture, v_uv + vec2(v_outline_thickness, v_outline_thickness)).a;
            outline += texture(u_texture, v_uv + vec2(-v_outline_thickness, -v_outline_thickness)).a;
            outline = min(outline, 1.0);
            vec4 c = texture(u_texture, v_uv);
            color = mix(c, v_outline_color, outline);
        } else {
            color = texture(u_texture, v_uv) * v_color;
        }
    }

    if (color.a < 0.5) discard;

    frag_color = color;
}