#version 330


layout(location = 0) in vec4 a_transform_col_0;
layout(location = 1) in vec4 a_transform_col_1;
layout(location = 2) in vec4 a_transform_col_2;
layout(location = 3) in vec4 a_transform_col_3;
layout(location = 4) in vec4 a_uv_offset_scale;
layout(location = 5) in vec4 a_color;
layout(location = 6) in int a_has_texture;
layout(location = 7) in int a_is_ui;

layout(std140) uniform UniformBuffer {
    mat4 view_projection;
    mat4 screen_projection;
} ubo;

out vec2 v_uv;
flat out vec4 v_color;
flat out int v_has_texture;

void main() {
    mat4 transform = mat4(a_transform_col_0, a_transform_col_1, a_transform_col_2, a_transform_col_3);

    vec2 position = vec2(
        gl_VertexID & 0x1,
        (gl_VertexID & 0x2) >> 1
    );

    mat4 projection = a_is_ui > 0 ? ubo.screen_projection : ubo.view_projection;

    gl_Position = projection * transform * vec4(position, 0.0, 1.0);

    v_uv = position * a_uv_offset_scale.zw + a_uv_offset_scale.xy;
    v_color = a_color;
    v_has_texture = a_has_texture;
}