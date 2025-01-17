#version 330 core

out vec4 frag_color;

uniform sampler2D u_texture;

in vec2 v_uv;
flat in vec4 v_color;
flat in uvec4 v_margin;
flat in vec2 v_source_size;
flat in vec2 v_output_size;

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

    vec4 color = texture(u_texture, new_uv) * v_color;

    if (color.a < 0.5) discard;

    frag_color = color;
}