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
    float2 position     [[attribute(0)]];
    float2 uv           [[attribute(1)]];
};

struct VertexOut
{
    float4 position     [[position]];
    float2 uv;
};

vertex VertexOut VS(
    VertexIn inp [[stage_in]],
    constant Constants& constants [[buffer(2)]]
) {
    VertexOut outp;
    outp.position = constants.view_projection * float4(position.x, position.y, 0.0, 1.0);
    outp.position.z = 1.0;
    outp.uv = inp.uv;

    return outp;
}

fragment float4 PS(
    VertexOut inp [[stage_in]],
    texture2d_array<float> static_lightmap_texture [[texture(3)]],
    sampler texture_sampler [[sampler(4)]]
) {
    const float4 color = static_lightmap_texture.sample(texture_sampler, inp.uv);
    return float4(color.rgb, 1.0);
}