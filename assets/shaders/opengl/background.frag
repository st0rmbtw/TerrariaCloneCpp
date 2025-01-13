#version 400 core

uniform sampler2DArray u_texture;

in vec2 v_uv;
flat in vec2 v_texture_size;
flat in vec2 v_size;
flat in vec2 v_tex_size;
flat in vec2 v_speed;
flat in vec2 v_offset;
flat in int v_nonscale;
flat in uint v_id;

// Bidirectional mod
float fmodb(float a, float b) {
    float r = mod(a, b);
    return r < 0.0 ? r + b : r;
}

layout(location = 0) out vec4 frag_color;

void main() {
    vec2 new_offset = vec2(-v_offset.x, v_offset.y);
    bool nonscale = v_nonscale > 0;
    vec2 speed = v_speed;

    float tex_aspect = v_tex_size.x / v_tex_size.y;
    float frame_aspect = v_size.x / v_size.y;
    float aspect = tex_aspect / frame_aspect;

    vec2 scale = vec2(1.0 / aspect, 1.0);

    if (!nonscale) {
        speed.x = speed.x / (v_size.x / v_tex_size.x);
    }

    vec2 uv = scale * v_uv - (new_offset * vec2(speed.x, 0.0));
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

    vec4 color = texture(u_texture, vec3(uv, float(v_id)));

    if (color.a <= 0.05) discard;

    frag_color = color;
}