#version 450 core

layout(location = 0) out vec4 frag_color;

layout(location = 0) in vec2 v_uv;
layout(location = 1) flat in vec2 v_size;
layout(location = 2) flat in vec2 v_tex_size;
layout(location = 3) flat in vec2 v_speed;
layout(location = 4) flat in int v_nonscale;

layout(set = 0, binding = 2) uniform texture2D u_texture;
layout(set = 0, binding = 3) uniform sampler u_sampler;

layout(binding = 1) uniform GlobalUniformBuffer {
    mat4 screen_projection;
    mat4 view_projection;
    mat4 nonscale_view_projection;
    mat4 nonscale_projection;
    mat4 transform_matrix;
    vec2 camera_position;
    vec2 window_size;
} ubo;

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
    int nonscale
) {
    vec2 new_offset = vec2(-offset.x, offset.y);

    if (nonscale <= 0) {
        speed.x = speed.x / (size.x / tex_size.x);
    }

    vec2 new_uv = uv - (new_offset * vec2(speed.x, 0.0));
    if (nonscale <= 0) {
        new_uv = new_uv * (size / tex_size);
    }

    new_uv.x = fmodb(new_uv.x, 1.0);
    new_uv.y = fmodb(new_uv.y, 1.0);

    return texture(sampler2D(u_texture, u_sampler), new_uv);
}

void main() {
    mat4 mvp = ubo.nonscale_projection * ubo.transform_matrix;

    vec4 clip = mvp * vec4(ubo.camera_position, 0.0, 1.0);
    vec2 offset = clip.xy;

    vec4 color = scroll(v_speed, v_uv, offset, v_tex_size, v_size, v_nonscale);

    if (color.a == 0.0) discard;

    frag_color = color;
}