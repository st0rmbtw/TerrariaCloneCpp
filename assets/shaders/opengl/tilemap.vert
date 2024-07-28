#version 330 core

layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_atlas_pos;
layout(location = 2) in uint a_tile_id;
layout(location = 3) in uint a_tile_type;

out VS_OUT {
    vec2 atlas_pos;
    flat uint tile_type;
    flat uint tile_id;
} vs_out;

void main() {
    vs_out.atlas_pos = a_atlas_pos;
    vs_out.tile_id = a_tile_id;
    vs_out.tile_type = a_tile_type;

    gl_Position = vec4(a_position * 16.0, 0, 1);
}