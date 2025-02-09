#version 450 core

layout(location = 0) in vec2 a_position;
layout(location = 1) in vec3 i_position;
layout(location = 2) in vec4 i_rotation;
layout(location = 3) in vec2 i_size;
layout(location = 4) in vec2 i_offset;
layout(location = 5) in vec4 i_uv_offset_scale;
layout(location = 6) in vec4 i_color;
layout(location = 7) in vec4 i_outline_color;
layout(location = 8) in float i_outline_thickness;
layout(location = 9) in int i_flags;

layout(binding = 2) uniform GlobalUniformBuffer {
    mat4 screen_projection;
    mat4 view_projection;
    mat4 nonscale_view_projection;
    mat4 nonscale_projection;
    mat4 transform_matrix;
    mat4 inv_view_proj;
    vec2 camera_position;
    vec2 window_size;
} uniforms;

const int IS_UI_FLAG = 1 << 0;
const int IS_NONSCALE_FLAG = 1 << 1;

layout(location = 0) out vec2 v_uv;
layout(location = 1) flat out vec4 v_color;
layout(location = 2) flat out vec4 v_outline_color;
layout(location = 3) flat out float v_outline_thickness;

void main() {
    float qxx = i_rotation.x * i_rotation.x;
    float qyy = i_rotation.y * i_rotation.y;
    float qzz = i_rotation.z * i_rotation.z;
    float qxz = i_rotation.x * i_rotation.z;
    float qxy = i_rotation.x * i_rotation.y;
    float qyz = i_rotation.y * i_rotation.z;
    float qwx = i_rotation.w * i_rotation.x;
    float qwy = i_rotation.w * i_rotation.y;
    float qwz = i_rotation.w * i_rotation.z;

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
        vec4(i_position.x, i_position.y, 0.0, 1.0)
    );

    transform *= rotation_matrix;

    vec2 offset = -i_offset * i_size;

    // translate
    transform[3] = transform[0] * offset[0] + transform[1] * offset[1] + transform[2] * 0.0 + transform[3];

    // scale
    transform[0] = transform[0] * i_size[0];
    transform[1] = transform[1] * i_size[1];

    bool is_ui = (i_flags & IS_UI_FLAG) == IS_UI_FLAG;
    bool is_nonscale = (i_flags & IS_NONSCALE_FLAG) == IS_NONSCALE_FLAG;

    mat4 mvp = (is_ui ? uniforms.screen_projection : is_nonscale ? uniforms.nonscale_view_projection : uniforms.view_projection) * transform;

    v_uv = a_position * i_uv_offset_scale.zw + i_uv_offset_scale.xy;
    v_color = i_color;
    v_outline_color = i_outline_color;
    v_outline_thickness = i_outline_thickness;
    
    gl_Position = mvp * vec4(a_position, 0, 1);
    gl_Position.z = i_position.z;
}