#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct Constants
{
    float4x4 screen_projection;
    float4x4 view_projection;
    float4x4 nonscale_view_projection;
    float4x4 nonscale_projection;
    float4x4 transform_matrix;
    float2 camera_position;
    float2 window_size;
    float max_depth;
    float max_world_depth;
};

struct Depth {
    float tile_depth;
    float wall_depth;
};

struct VertexIn
{
    float2 position      [[attribute(0)]];
    float2 wall_tex_size [[attribute(1)]];
    float2 tile_tex_size [[attribute(2)]];
    float2 wall_padding  [[attribute(3)]];
    float2 tile_padding  [[attribute(4)]];

    float2 i_position    [[attribute(5)]];
    float2 i_atlas_pos   [[attribute(6)]];
    float2 i_world_pos   [[attribute(7)]];
    uint   i_tile_id     [[attribute(8)]];
    uint   i_tile_type   [[attribute(9)]];
};

struct VertexOut
{
    float4 position [[position]];
    float2 uv;
    uint tile_id [[flat]];
};

constant constexpr uint TILE_TYPE_WALL = 1;

vertex VertexOut VS(
    VertexIn inp [[stage_in]],
    constant Constants& constants [[buffer(1)]],
    constant Depth& depth [[buffer(2)]]
) {
    const float2 world_pos = inp.i_world_pos;
    const uint tile_type = inp.i_tile_type;

    float order = depth.tile_depth;
    float2 size = float2(TILE_SIZE, TILE_SIZE);
    float2 start_uv = inp.i_atlas_pos * (inp.tile_tex_size + inp.tile_padding);
    float2 tex_size = inp.tile_tex_size;

    if (tile_type == TILE_TYPE_WALL) {
        order = depth.wall_depth;
        size = float2(WALL_SIZE, WALL_SIZE);
        start_uv = inp.i_atlas_pos * (inp.wall_tex_size + inp.wall_padding);
        tex_size = inp.wall_tex_size;
    }

    order /= constants.max_world_depth;

    const float4x4 transform = float4x4(
        float4(1.0, 0.0, 0.0, 0.0),
        float4(0.0, 1.0, 0.0, 0.0),
        float4(0.0, 0.0, 1.0, 0.0),
        float4(world_pos.x, world_pos.y, 0.0, 1.0)
    );

    const float4x4 mvp = constants.view_projection * transform;
    const float2 position = inp.i_position * 16.0 + inp.position * size;

	VertexOut output;
    output.uv = start_uv + inp.position * tex_size;
    output.tile_id = inp.i_tile_id;
    output.position = mvp * float4(position, 0.0, 1.0);
    output.position.z = 1.0;

	return output;
}

fragment float4 PS(
    VertexOut inp [[stage_in]],
    texture2d_array<float> texture [[texture(3)]],
    sampler texture_sampler [[sampler(4)]]
) {
    const float4 color = texture.sample(texture_sampler, inp.uv, inp.tile_id);

    if (color.a < 0.5) discard_fragment();

    return color;
}