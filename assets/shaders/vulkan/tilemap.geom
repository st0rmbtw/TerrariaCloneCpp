#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform UniformBuffer {
    mat4 screen_projection;
    mat4 view_projection;
    mat4 nonscale_view_projection;
} ubo;

layout(binding = 2) uniform OrderBuffer {
    float tile_order;
    float wall_order;
} ubo2;

layout(location = 0) in VS_OUT {
    vec4 uv_size;
    vec2 world_pos;
    flat uint tile_type;
    flat uint tile_id;
} gs_in[];  

layout(location = 0) out vec2 g_uv;
layout(location = 1) flat out uint g_tile_id;

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

const uint TILE_TYPE_WALL = 1u;

void main() {
    vec2 start_uv = gs_in[0].uv_size.xy;
    vec2 tex_size = gs_in[0].uv_size.zw;
    vec2 world_pos = gs_in[0].world_pos;
    uint tile_id = gs_in[0].tile_id;
    uint tile_type = gs_in[0].tile_type;

    float order = ubo2.tile_order;
    vec2 size = vec2(TILE_SIZE);

    if (tile_type == TILE_TYPE_WALL) {
        order = ubo2.wall_order;
        size = vec2(WALL_SIZE);
    }

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
    g_uv = start_uv + tex_size;
    g_tile_id = tile_id;
    EmitVertex();

    EndPrimitive();
}