#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct Constants
{
    float4x4 u_view_projection;
};

struct VertexIn
{
    float4 transform0      [[attribute(0)]];
    float4 transform1      [[attribute(1)]];
    float4 transform2      [[attribute(2)]];
    float4 transform3      [[attribute(3)]];
    float4 uv_offset_scale [[attribute(4)]];
    float4 color           [[attribute(5)]];
};

struct VertexOut
{
    float4 position [[position]];
    float4 uv_offset_scale;
    float4 color;
};

vertex VertexOut VS(
    VertexIn inp [[stage_in]],
    constant Constants& constants [[buffer(0)]]
) {
    VertexOut outp;

    outp.position = constants.u_view_projection * inp.transform * float4(inp.position, 0.0, 1.0);
    outp.uv_offset_scale = inp.uv_offset_scale;
    outp.color = inp.color;

    return outp;
}

fragment float4 PS(VertexOut inp [[stage_in]])
{
    return inp.color;
}