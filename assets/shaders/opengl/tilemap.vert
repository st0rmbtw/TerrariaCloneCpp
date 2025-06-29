#version 330 core

layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 i_atlas_pos;
layout(location = 2) in vec2 i_world_pos;
layout(location = 3) in uint i_position;
layout(location = 4) in uint i_tile_data;

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

struct TileData {
    vec2 tex_size;
    vec2 tex_padding;
    vec2 size;
    vec2 offset;
    float depth;
};

layout(std140) uniform TileDataBuffer {
    TileData data[3];
} tile_data_buffer;

out vec2 v_uv;
flat out uint v_tile_id;

vec2 unpack_position(uint position) {
    uint x = position & 0xFFu;
    uint y = (position >> 8) & 0xFFu;
    return vec2(float(x), float(y));
}

void main() {
    // Extract last 6 bits
    uint tile_type = i_tile_data & 0x3fu;
    // Extract other 10 bits
    uint tile_id = (i_tile_data >> 6) & 0x3ffu;

    TileData tile_data = tile_data_buffer.data[tile_type];
    float depth = tile_data.depth;
    vec2 size = tile_data.size;
    vec2 tex_size = size / tile_data.tex_size;
    vec2 start_uv = i_atlas_pos * (tex_size + tile_data.tex_padding);
    vec2 tex_dims = tile_data.tex_size;
    vec2 offset = tile_data.offset;

    mat4 transform = mat4(
        vec4(1.0, 0.0, 0.0, 0.0),
        vec4(0.0, 1.0, 0.0, 0.0),
        vec4(0.0, 0.0, 1.0, 0.0),
        vec4(i_world_pos, 0.0, 1.0)
    );

    mat4 mvp = global_ubo.view_projection * transform;
    vec2 position = unpack_position(i_position) * 16.0 + a_position * size + offset;
    vec2 uv = start_uv + a_position * tex_size;

    vec2 pixel_offset = vec2(0.1 / tex_dims.x, 0.1 / tex_dims.y);

    v_uv = uv + pixel_offset * (vec2(1.0, 1.0) - a_position * 2.0);
    v_tile_id = tile_id;
    gl_Position = mvp * vec4(position, 0.0, 1.0);
    gl_Position.z = depth;
}