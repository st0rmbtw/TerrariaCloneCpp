#version 450 core

layout(set = 0, binding = 3) uniform texture2DArray u_texture;
layout(set = 0, binding = 4) uniform sampler u_sampler;

layout(location = 0) in vec2 v_uv;
layout(location = 1) flat in vec2 v_texture_size;
layout(location = 2) flat in vec2 v_size;
layout(location = 3) flat in vec2 v_tex_size;
layout(location = 4) flat in vec2 v_speed;
layout(location = 5) flat in vec2 v_offset;
layout(location = 6) flat in int v_nonscale;
layout(location = 7) flat in uint v_id;

// Bidirectional mod
float fmodb(float a, float b) {
    float r = mod(a, b);
    return r < 0.0 ? r + b : r;
}

layout(location = 0) out vec4 frag_color;

void main() {
    const vec2 new_offset = vec2(-v_offset.x, v_offset.y);
    const bool nonscale = v_nonscale > 0;
    vec2 speed = v_speed;

    if (!nonscale) {
        speed.x = speed.x / (v_size.x / v_tex_size.x);
    }

    vec2 uv = v_uv - (new_offset * vec2(speed.x, 0.0));
    if (!nonscale) {
        uv = uv * (v_size / v_tex_size);
    }

    uv.x = fmodb(uv.x, 1.0);
    uv.y = fmodb(uv.y, 1.0);

    if (v_id == 0) {
        float x = uv.x;
        uv.x = uv.y;
        uv.y = x;
    }

    uv = uv * v_tex_size / v_texture_size;

    const vec4 color = texture(sampler2DArray(u_texture, u_sampler), vec3(uv, float(v_id)));

    if (color.a <= 0.05) discard;

    frag_color = color;
}