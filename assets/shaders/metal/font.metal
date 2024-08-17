#include <metal_stdlib>

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
    float max_depth;
    float max_world_depth;
};

struct VertexIn
{
    float2 position   [[attribute(0)]];

    float3 i_color    [[attribute(1)]];
    float3 i_position [[attribute(2)]];
    float2 i_size     [[attribute(3)]];
    float2 i_tex_size [[attribute(4)]];
    float2 i_uv       [[attribute(5)]];
    int    i_is_ui    [[attribute(6)]];
};

struct VertexOut
{
    float4 position [[position]];
    float3 color [[flat]];
    float2 uv;
};

vertex VertexOut VS(
    VertexIn inp [[stage_in]],
    constant Constants& constants [[buffer(2)]]
) {
    const float4x4 mvp = inp.i_is_ui > 0 ? constants.screen_projection : constants.view_projection;

    const float2 position = inp.i_position.xy + inp.position * inp.i_size;
    const float2 uv = inp.i_uv + inp.position * inp.i_tex_size;

	VertexOut outp;
    outp.color = inp.i_color;
    outp.uv = uv;
    outp.position = mvp * float4(position, 0.0, 1.0);
    outp.position.z = inp.i_position.z / constants.max_depth;

    return outp;
}

fragment float4 PS(
    VertexOut inp [[stage_in]],
    texture2d<float> texture [[texture(3)]],
    sampler texture_sampler [[sampler(4)]]
) {
    const float4 color = float4(inp.color, texture.sample(texture_sampler, inp.uv).r);

    if (color.a < 0.05) discard_fragment();

    return color;
}