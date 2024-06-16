#version 330


layout(location = 0) in vec4 a_transform_col_0;
layout(location = 1) in vec4 a_transform_col_1;
layout(location = 2) in vec4 a_transform_col_2;
layout(location = 3) in vec4 a_transform_col_3;
layout(location = 4) in vec4 a_uv_offset_scale;
layout(location = 5) in vec4 a_color;

uniform mat4 u_view_projection;

out vec2 v_uv;
flat out vec4 v_color;

void main() {
    mat4 transform = mat4(a_transform_col_0, a_transform_col_1, a_transform_col_2, a_transform_col_3);

    vec2 position = vec2(
        gl_VertexID & 0x1,
        (gl_VertexID & 0x2) >> 1
    );

    gl_Position = u_view_projection * transform * vec4(position, 0.0, 1.0);

    v_uv = position * a_uv_offset_scale.zw + a_uv_offset_scale.xy;
    v_color = a_color;
}