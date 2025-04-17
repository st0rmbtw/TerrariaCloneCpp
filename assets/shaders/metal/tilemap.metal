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
    float4x4 inv_view_proj;
    float2 camera_position;
    float2 window_size;
};

struct TileData {
    float2 tex_size;
    float2 tex_padding;
    float2 size;
    float2 offset;
    float depth;
};

struct TileDataBuffer {
    TileData data[3];
};

struct VertexIn
{
    float2 position      [[attribute(0)]];

    uint   i_position    [[attribute(1)]];
    float2 i_atlas_pos   [[attribute(2)]];
    float2 i_world_pos   [[attribute(3)]];
    uint   i_tile_data   [[attribute(4)]];
};

struct VertexOut
{
    float4 position [[position]];
    float2 uv;
    uint tile_id [[flat]];
};

constant constexpr uint TILE_TYPE_WALL = 1u;
constant constexpr uint TILE_TYPE_TORCH = 2u;

static float2 unpack_position(uint position) {
    uint x = position & 0xFF;
    uint y = (position >> 8) & 0xFF;
    return float2(float(x), float(y));
}

vertex VertexOut VS(
    VertexIn inp [[stage_in]],
    constant Constants& constants [[buffer(2)]],
    constant TileDataBuffer& tile_data_buffer [[buffer(3)]]
) {
    const float2 world_pos = inp.i_world_pos;

    // Extract last 6 bits
    const uint tile_type = inp.i_tile_data & 0x3f;
    // Extract other 10 bits
    const uint tile_id = (inp.i_tile_data >> 6) & 0x3ff;

    const TileData tile_data = tile_data_buffer.data[tile_type];
    const float depth = tile_data.depth;
    const float2 size = tile_data.size;
    const float2 tex_size = size / tile_data.tex_size;
    const float2 start_uv = inp.i_atlas_pos * (tex_size + tile_data.tex_padding);
    const float2 tex_dims = tile_data.tex_size;
    const float2 offset = tile_data.offset;

    const float4x4 transform = float4x4(
        float4(1.0, 0.0, 0.0, 0.0),
        float4(0.0, 1.0, 0.0, 0.0),
        float4(0.0, 0.0, 1.0, 0.0),
        float4(world_pos.x, world_pos.y, 0.0, 1.0)
    );

    const float4x4 mvp = constants.view_projection * transform;
    const float2 position = unpack_position(inp.i_position) * 16.0 + inp.position * size + offset;
    const float2 uv = start_uv + inp.position * tex_size;

    const float2 pixel_offset = float2(0.1 / tex_dims.x, 0.1 / tex_dims.y);

	VertexOut output;
    output.uv = uv + pixel_offset * (float2(1.0, 1.0) - inp.position * 2.0);
    output.tile_id = tile_id;
    output.position = mvp * float4(position, 0.0, 1.0);
    output.position.z = depth;

	return output;
}

fragment float4 PS(
    VertexOut inp [[stage_in]],
    texture2d_array<float> texture [[texture(4)]],
    sampler texture_sampler [[sampler(5)]]
) {
    const float4 color = texture.sample(texture_sampler, inp.uv, inp.tile_id);

    if (color.a < 0.5) discard_fragment();

    return color;
}