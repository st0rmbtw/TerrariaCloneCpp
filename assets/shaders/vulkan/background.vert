#version 450 core

layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_texture_size;
layout(location = 2) in vec2 i_position;
layout(location = 3) in vec2 i_size;
layout(location = 4) in vec2 i_tex_size;
layout(location = 5) in vec2 i_speed;
layout(location = 6) in uint i_id;
layout(location = 7) in int i_flags;

layout(binding = 2) uniform GlobalUniformBuffer {
    mat4 screen_projection;
    mat4 view_projection;
    mat4 nonscale_view_projection;
    mat4 nonscale_projection;
    mat4 transform_matrix;
    mat4 inv_view_proj;
    vec2 camera_position;
    vec2 window_size;
    float max_depth;
    float max_world_depth;
} global_ubo;

layout(location = 0) out vec2 v_uv;
layout(location = 1) flat out vec2 v_texture_size;
layout(location = 2) flat out vec2 v_size;
layout(location = 3) flat out vec2 v_tex_size;
layout(location = 4) flat out vec2 v_speed;
layout(location = 5) flat out vec2 v_offset;
layout(location = 6) flat out int v_nonscale;
layout(location = 7) flat out uint v_id;

const int IGNORE_CAMERA_ZOOM_FLAG = 1 << 0;

void main() {
    const int flags = i_flags;
    const bool ignore_camera_zoom = (flags & IGNORE_CAMERA_ZOOM_FLAG) == IGNORE_CAMERA_ZOOM_FLAG;

    const mat4 view_proj = ignore_camera_zoom ? global_ubo.nonscale_view_projection : global_ubo.view_projection;
    const mat4 proj_model = global_ubo.nonscale_projection * global_ubo.transform_matrix;

    const vec2 offset = (proj_model * vec4(global_ubo.camera_position.x, global_ubo.camera_position.y, 0.0, 1.0)).xy;

    const vec2 position = i_position.xy + i_size * a_position;

    gl_Position = view_proj * vec4(position, 0.0, 1.0);
    gl_Position.z = 0.0;
    v_texture_size = a_texture_size;
    v_uv = a_position;
    v_size = i_size;
    v_tex_size = i_tex_size;
    v_speed = i_speed;
    v_offset = offset;
    v_id = i_id;
    v_nonscale = flags & IGNORE_CAMERA_ZOOM_FLAG;

    if (i_id == 0) {
        v_texture_size.x = a_texture_size.y;
        v_texture_size.y = a_texture_size.x;
    }
}