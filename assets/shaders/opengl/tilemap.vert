#version 330 core

layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_wall_tex_size;
layout(location = 2) in vec2 a_tile_tex_size;
layout(location = 3) in vec2 a_wall_padding;
layout(location = 4) in vec2 a_tile_padding;
layout(location = 5) in vec2 i_position;
layout(location = 6) in vec2 i_atlas_pos;
layout(location = 7) in vec2 i_world_pos;
layout(location = 8) in uint i_tile_data;

layout(std140) uniform GlobalUniformBuffer {
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

layout(std140) uniform DepthBuffer {
    float tile_depth;
    float wall_depth;
} depth_ubo;

const uint TILE_TYPE_WALL = 1u;

out vec2 v_uv;
flat out uint v_tile_id;

void main() {
    // Extract last 6 bits
    uint tile_type = i_tile_data & 0x3fu;
    // Extract other 10 bits
    uint tile_id = (i_tile_data >> 6) & 0x3ffu;

    float order = depth_ubo.tile_depth;
    vec2 size = vec2(TILE_SIZE, TILE_SIZE);
    vec2 tex_size = size / a_tile_tex_size;
    vec2 start_uv = i_atlas_pos * (tex_size + a_tile_padding);
    vec2 tex_dims = a_tile_tex_size;

    if (tile_type == TILE_TYPE_WALL) {
        order = depth_ubo.wall_depth;
        size = vec2(WALL_SIZE, WALL_SIZE);
        tex_size = size / a_wall_tex_size;
        start_uv = i_atlas_pos * (tex_size + a_wall_padding);
        tex_dims = a_wall_tex_size;
    }

    order /= global_ubo.max_world_depth;

    mat4 transform = mat4(
        vec4(1.0, 0.0, 0.0, 0.0),
        vec4(0.0, 1.0, 0.0, 0.0),
        vec4(0.0, 0.0, 1.0, 0.0),
        vec4(i_world_pos, 0.0, 1.0)
    );

    mat4 mvp = global_ubo.view_projection * transform;
    vec2 position = i_position * 16.0 + a_position * size;
    vec2 uv = start_uv + a_position * tex_size;

    vec2 pixel_offset = vec2(0.1 / tex_dims.x, 0.1 / tex_dims.y);

    v_uv = uv + pixel_offset * (vec2(1.0, 1.0) - a_position * 2.0);
    v_tile_id = tile_id;
    gl_Position = mvp * vec4(position, 0.0, 1.0);
    gl_Position.z = order;
}