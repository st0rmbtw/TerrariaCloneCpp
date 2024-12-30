#version 450 core

layout(location = 0) out vec4 frag_color;

layout(location = 0) in vec2 v_uv;
layout(location = 1) flat in vec3 v_color;

layout(set = 0, binding = 3) uniform texture2D u_texture;
layout(set = 0, binding = 4) uniform sampler u_sampler;

const float GLYPH_CENTER = 0.5;

void main() {
    const float outline = 0.2;

    const float dist = texture(sampler2D(u_texture, u_sampler), v_uv).r;
    const float width = fwidth(dist);
    const float alpha = smoothstep(GLYPH_CENTER - outline - width, GLYPH_CENTER - outline + width, abs(dist));
    // vec4 color = vec4(v_color, alpha);

    const float mu = smoothstep(0.5-width, 0.5+width, abs(dist));
    const vec3 rgb = mix(vec3(0.0, 0.0, 0.0), v_color, mu);
    const vec4 color = vec4(rgb, alpha);

    if (color.a <= 0.05) discard;

    frag_color = color;
}