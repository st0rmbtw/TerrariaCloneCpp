#version 330 core

layout(location = 0)  in vec2 a_position;
layout(location = 1)  in vec2 i_position;
layout(location = 2)  in vec4 i_rotation;
layout(location = 3)  in vec2 i_size;
layout(location = 4)  in vec2 i_offset;
layout(location = 5)  in vec2 i_source_size;
layout(location = 6)  in vec2 i_output_size;
layout(location = 7)  in uvec4 i_margin;
layout(location = 8)  in vec4 i_uv_offset_scale;
layout(location = 9)  in vec4 i_color;
layout(location = 10) in int i_flags;

layout(std140) uniform GlobalUniformBuffer {
    mat4 screen_projection;
    mat4 view_projection;
    mat4 nonscale_view_projection;
    mat4 nonscale_projection;
    mat4 transform_matrix;
    mat4 inv_view_proj;
    vec2 camera_position;
    vec2 window_size;
} global_ubo;

const int HAS_TEXTURE_FLAG = 1 << 0;
const int IS_UI_FLAG = 1 << 1;
const int IS_WORLD_FLAG = 1 << 2;
const int IS_NONSCALE_FLAG = 1 << 3;

out vec2 v_uv;
flat out vec4 v_color;
flat out uvec4 v_margin;
flat out vec2 v_source_size;
flat out vec2 v_output_size;

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

    mat4 mvp = (is_ui ? global_ubo.screen_projection : global_ubo.view_projection) * transform;

    v_uv = a_position * i_uv_offset_scale.zw + i_uv_offset_scale.xy;
    v_color = i_color;
    v_margin = i_margin;
    v_source_size = i_source_size;
    v_output_size = i_output_size;
    
    gl_Position = mvp * vec4(a_position, 0, 1);
    gl_Position.z = 1.0;
}