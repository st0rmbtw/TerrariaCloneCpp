#version 330 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec4 a_rotation;
layout(location = 2) in vec2 a_size;
layout(location = 3) in vec2 a_offset;
layout(location = 4) in vec4 a_uv_offset_scale;
layout(location = 5) in vec4 a_color;
layout(location = 6) in vec4 a_outline_color;
layout(location = 7) in float a_outline_thickness;
layout(location = 8) in int a_has_texture;
layout(location = 9) in int a_is_ui;
layout(location = 10) in int a_is_nonscale;

out VS_OUT {
    mat4 transform;
    vec4 uv_offset_scale;
    vec4 color;
    vec4 outline_color;
    float outline_thickness;
    float order;
    int has_texture;
    bool is_ui;
    bool is_nonscale;
} vs_out;

void main() {
    float qxx = a_rotation.x * a_rotation.x;
    float qyy = a_rotation.y * a_rotation.y;
    float qzz = a_rotation.z * a_rotation.z;
    float qxz = a_rotation.x * a_rotation.z;
    float qxy = a_rotation.x * a_rotation.y;
    float qyz = a_rotation.y * a_rotation.z;
    float qwx = a_rotation.w * a_rotation.x;
    float qwy = a_rotation.w * a_rotation.y;
    float qwz = a_rotation.w * a_rotation.z;

    mat4 rotation_matrix = mat4(
        vec4(1.0 - 2.0 * (qyy + qzz), 2.0 * (qxy + qwz), 2.0 * (qxz - qwy), 0.0),
        vec4(2.0 * (qxy - qwz), 1.0 - 2.0 * (qxx +  qzz), 2.0 * (qyz + qwx), 0.0),
        vec4(2.0 * (qxz + qwy), 2.0 * (qyz - qwx), 1.0 - 2.0 * (qxx +  qyy), 0.0),
        vec4(0.0, 0.0, 0.0, 1.0)
    );

    mat4 transform = mat4(
        vec4(1.0, 0.0, 0.0, 0.0),
        vec4(0.0, 1.0, 0.0, 0.0),
        vec4(0.0, 0.0, 1.0, 0.0),
        vec4(a_position.x, a_position.y, 0.0, 1.0)
    );

    transform *= rotation_matrix;

    vec2 offset = -a_offset * a_size;

    // translate
    transform[3] = transform[0] * offset[0] + transform[1] * offset[1] + transform[2] * 0.0 + transform[3];

    // scale
    transform[0] = transform[0] * a_size[0];
    transform[1] = transform[1] * a_size[1];

    gl_Position = vec4(1.0);

    vs_out.transform = transform;
    vs_out.uv_offset_scale = a_uv_offset_scale;
    vs_out.color = a_color;
    vs_out.outline_color = a_outline_color;
    vs_out.outline_thickness = a_outline_thickness;
    vs_out.order = a_position.z;
    vs_out.has_texture = a_has_texture;
    vs_out.is_ui = a_is_ui > 0;
    vs_out.is_nonscale = a_is_nonscale > 0;
}