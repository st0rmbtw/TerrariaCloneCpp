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
    float4 transform0        [[attribute(0)]];
    float4 transform1        [[attribute(1)]];
    float4 transform2        [[attribute(2)]];
    float4 transform3        [[attribute(3)]];
    float4 uv_offset_scale   [[attribute(4)]];
    float4 color             [[attribute(5)]];
    float4 outline_color     [[attribute(6)]];
    float  outline_thickness [[attribute(7)]];
    
    int has_texture          [[attribute(8)]];
    int is_ui                [[attribute(9)]];
};

struct VertexOut
{
    float4 position         [[position]];
    float4 uv_offset_scale  [[flat]];
    float4 color            [[flat]];
    float4 outline_color    [[flat]];
    float outline_thickness [[flat]];
    int has_texture         [[flat]];
    int is_ui               [[flat]];
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
    outp.outline_color = inp.outline_color;
    outp.outline_thickness = inp.outline_thickness;
    outp.has_texture = inp.has_texture;

    return outp;
}

fragment float4 PS(
    VertexOut inp [[stage_in]],
    texture2d<float> texture [[texture(2)]],
    sampler texture_sampler [[sampler(3)]]
) {
    if (inp.has_texture > 0) {
        if (inp.outline_thickness > 0.0) {
            float outline = texture.sample(texture_sampler, inp.uv + float2(inp.outline_thickness, 0.0)).a;
            outline += texture.sample(texture_sampler, inp.uv + float2(-inp.outline_thickness, 0.0)).a;
            outline += texture.sample(texture_sampler, inp.uv + float2(0.0, inp.outline_thickness)).a;
            outline += texture.sample(texture_sampler, inp.uv + float2(0.0, -inp.outline_thickness)).a;
            outline += texture.sample(texture_sampler, inp.uv + float2(inp.outline_thickness, -inp.outline_thickness)).a;
            outline += texture.sample(texture_sampler, inp.uv + float2(-inp.outline_thickness, inp.outline_thickness)).a;
            outline += texture.sample(texture_sampler, inp.uv + float2(inp.outline_thickness, inp.outline_thickness)).a;
            outline += texture.sample(texture_sampler, inp.uv + float2(-inp.outline_thickness, -inp.outline_thickness)).a;
            outline = min(outline, 1.0);
            float4 c = texture.sample(texture_sampler, inp.uv);
            return mix(c, inp.outline_color, outline);
        } else {
            return texture.sample(texture_sampler, inp.uv) * inp.color;
        }
    } else {
        return inp.color;
    }
}