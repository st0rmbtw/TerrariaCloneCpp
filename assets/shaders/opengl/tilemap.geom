#version 330 core

layout(std140) uniform UniformBuffer {
    mat4 screen_projection;
    mat4 view_projection;
    mat4 nonscale_view_projection;
} ubo;

layout(std140) uniform OrderBuffer {
    float tile_order;
    float wall_order;
} ubo2;

in VS_OUT {
    vec2 atlas_pos;
    vec2 world_pos;
    flat uint tile_type;
    flat uint tile_id;
} gs_in[];  

out vec2 g_uv;
flat out uint g_tile_id;

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

const vec2 TILE_TEX_PADDING = vec2(TILE_TEXTURE_PADDING) / vec2(TILE_TEXTURE_WIDTH, TILE_TEXTURE_HEIGHT);
const vec2 WALL_TEX_PADDING = vec2(WALL_TEXTURE_PADDING) / vec2(WALL_TEXTURE_WIDTH, WALL_TEXTURE_HEIGHT);

const vec2 TILE_TEX_SIZE = vec2(TILE_SIZE) / vec2(TILE_TEXTURE_WIDTH, TILE_TEXTURE_HEIGHT);
const vec2 WALL_TEX_SIZE = vec2(WALL_SIZE) / vec2(WALL_TEXTURE_WIDTH, WALL_TEXTURE_HEIGHT);

const uint TILE_TYPE_WALL = 1u;

void main() {
    vec2 atlas_pos = gs_in[0].atlas_pos;
    vec2 world_pos = gs_in[0].world_pos;
    uint tile_id = gs_in[0].tile_id;
    uint tile_type = gs_in[0].tile_type;

    float order = ubo2.tile_order;
    vec2 size = vec2(TILE_SIZE);
    vec2 tex_size = vec2(TILE_TEX_SIZE);
    vec2 padding = TILE_TEX_PADDING;

    if (tile_type == TILE_TYPE_WALL) {
        order = ubo2.wall_order;
        size = vec2(WALL_SIZE);
        tex_size = vec2(WALL_TEX_SIZE);
        padding = WALL_TEX_PADDING;
    }

    vec2 start_uv = atlas_pos * (tex_size + padding);

    mat4 transform = mat4(
        vec4(1.0, 0.0, 0.0, 0.0),
        vec4(0.0, 1.0, 0.0, 0.0),
        vec4(0.0, 0.0, 1.0, 0.0),
        vec4(world_pos.x, world_pos.y, 0.0, 1.0)
    );

    mat4 mvp = ubo.view_projection * transform;

    gl_Position = mvp * gl_in[0].gl_Position;
    gl_Position.z = order;
    g_uv = start_uv;
    g_tile_id = tile_id;
    EmitVertex();

    gl_Position = mvp * (gl_in[0].gl_Position + vec4(size.x, 0.0, 0.0, 0.0));
    gl_Position.z = order;
    g_uv = vec2(start_uv.x + tex_size.x, start_uv.y);
    g_tile_id = tile_id;
    EmitVertex();

    gl_Position = mvp * (gl_in[0].gl_Position + vec4(0.0, size.y, 0.0, 0.0));
    gl_Position.z = order;
    g_uv = vec2(start_uv.x, start_uv.y + tex_size.y);
    g_tile_id = tile_id;
    EmitVertex();

    gl_Position = mvp * (gl_in[0].gl_Position + vec4(size, 0.0, 0.0));
    gl_Position.z = order;
    g_uv = vec2(start_uv.x + tex_size.x, start_uv.y + tex_size.y);
    g_tile_id = tile_id;
    EmitVertex();

    EndPrimitive();
}