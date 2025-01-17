#version 430 core

layout(location = 0) out vec4 frag_color;

layout(binding = 3) uniform texture2D u_texture;
layout(binding = 4) uniform sampler u_sampler;

layout(location = 0) in vec2 v_uv;
layout(location = 1) flat in vec4 v_color;
layout(location = 2) flat in uvec4 v_margin;
layout(location = 3) flat in vec2 v_source_size;
layout(location = 4) flat in vec2 v_output_size;

float map(float value, float in_min, float in_max, float out_min, float out_max) {
    return (value - in_min) / (in_max - in_min) * (out_max - out_min) + out_min;
} 

float process_axis(float coord, vec2 source_margin, vec2 out_margin) {
    if (coord < out_margin.x) {
        return map(coord, 0, out_margin.x, 0, source_margin.x);
    }
    if (coord < 1. - out_margin.y) {
        return map(coord, out_margin.x, 1. - out_margin.y, source_margin.x, 1. - source_margin.y);
    }
    return map(coord, 1. - out_margin.y, 1, 1. - source_margin.y, 1);
}

void main() {
    vec2 horizontal_margin = v_margin.xy;
    vec2 vertical_margin = v_margin.zw;

    vec2 new_uv = vec2(
        process_axis(v_uv.x,
            horizontal_margin / v_source_size.xx,
            horizontal_margin / v_output_size.xx
        ),
        process_axis(v_uv.y,
            vertical_margin / v_source_size.yy,
            vertical_margin / v_output_size.yy
        )
    );

    vec4 color = texture(sampler2D(u_texture, u_sampler), new_uv) * v_color;

    if (color.a < 0.5) discard;

    frag_color = color;
}