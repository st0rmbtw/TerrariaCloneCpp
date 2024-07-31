#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct Constants
{
    float4x4 screen_projection;
    float4x4 projection;
    float4x4 view;
};

struct VertexIn
{
    float4 transform0      [[attribute(0)]];
    float4 transform1      [[attribute(1)]];
    float4 transform2      [[attribute(2)]];
    float4 transform3      [[attribute(3)]];
    float4 uv_offset_scale [[attribute(4)]];
    float4 color           [[attribute(5)]];
    int has_texture        [[attribute(6)]];
    int is_ui              [[attribute(7)]];
};

struct VertexOut
{
    float4 position [[position]];
    float4 uv_offset_scale;
    float4 color;
    int has_texture [[flat]];
    int is_ui       [[flat]];
};

vertex VertexOut VS(
    VertexIn inp [[stage_in]],
    constant Constants& constants [[buffer(1)]]
) {
    VertexOut outp;

    float4x4 projection = inp.is_ui > 0 ? constants.screen_projection : constants.projection * constants.view;

    outp.position = projection * inp.transform * float4(inp.position, 0.0, 1.0);
    outp.uv_offset_scale = inp.uv_offset_scale;
    outp.color = inp.color;
    outp.has_texture = inp.has_texture;

    return outp;
}

fragment float4 PS(
    VertexOut inp [[stage_in]],
    texture2d<float> texture [[texture(2)]],
    sampler texture_sampler [[sampler(3)]]
) {
    if (inp.has_texture > 0) {
        return texture.sample(texture_sampler, inp.uv) * inp.color;
    } else {
        return inp.color;
    }
}