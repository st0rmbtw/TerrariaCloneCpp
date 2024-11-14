#version 400 core

layout(location = 0) out vec4 frag_color;

in vec2 v_uv;
flat in vec2 v_size;
flat in vec2 v_tex_size;
flat in vec2 v_speed;
flat in vec2 v_offset;
flat in int v_nonscale;

uniform sampler2D u_texture;

// Bidirectional mod
float fmodb(float a, float b) {
    float r = mod(a, b);
    return r < 0.0 ? r + b : r;
}

vec4 scroll(
    vec2 speed,
    vec2 uv,
    vec2 offset,
    vec2 tex_size,
    vec2 size,
    bool nonscale
) {
    vec2 new_offset = vec2(-offset.x, offset.y);

    if (!nonscale) {
        speed.x = speed.x / (size.x / tex_size.x);
    }

    vec2 new_uv = uv - (new_offset * vec2(speed.x, 0.0));
    if (!nonscale) {
        new_uv = new_uv * (size / tex_size);
    }

    new_uv.x = fmodb(new_uv.x, 1.0);
    new_uv.y = fmodb(new_uv.y, 1.0);

    return texture(u_texture, new_uv);
}

void main() {
    vec4 color = scroll(v_speed, v_uv, v_offset, v_tex_size, v_size, v_nonscale > 0);

    if (color.a <= 0.05) discard;

    frag_color = color;
}