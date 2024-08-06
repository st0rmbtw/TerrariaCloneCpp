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
    float3 position          [[attribute(0)]];
    float4 rotation          [[attribute(1)]];
    float2 size              [[attribute(2)]];
    float2 offset            [[attribute(3)]];
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

    float qxx = inp.rotation.x * inp.rotation.x;
    float qyy = inp.rotation.y * inp.rotation.y;
    float qzz = inp.rotation.z * inp.rotation.z;
    float qxz = inp.rotation.x * inp.rotation.z;
    float qxy = inp.rotation.x * inp.rotation.y;
    float qyz = inp.rotation.y * inp.rotation.z;
    float qwx = inp.rotation.w * inp.rotation.x;
    float qwy = inp.rotation.w * inp.rotation.y;
    float qwz = inp.rotation.w * inp.rotation.z;

    float4x4 rotation_matrix = float4x4(
        {1.0 - 2.0 * (qyy + qzz), 2.0 * (qxy + qwz), 2.0 * (qxz - qwy), 0.0},
        {2.0 * (qxy - qwz), 1.0 - 2.0 * (qxx +  qzz), 2.0 * (qyz + qwx), 0.0},
        {2.0 * (qxz + qwy), 2.0 * (qyz - qwx), 1.0 - 2.0 * (qxx +  qyy), 0.0},
        {0.0, 0.0, 0.0, 1.0}
    );

    float4x4 transform = float4x4(
        {1.0, 0.0, 0.0, 0.0},
        {0.0, 1.0, 0.0, 0.0},
        {0.0, 0.0, 1.0, 0.0},
        {inp.position.x, inp.position.y, 0.0, 1.0}
    );

    transform *= rotation_matrix;

    float2 offset = -inp.offset * inp.size;

    // translate
    transform[3] = transform[0] * offset[0] + transform[1] * offset[1] + transform[2] * 0.0 + transform[3];

    // scale
    transform[0] = transform[0] * inp.size[0];
    transform[1] = transform[1] * inp.size[1];

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
    float4 color = inp.color;

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
            color = mix(c, inp.outline_color, outline);
        } else {
            color = texture.sample(texture_sampler, inp.uv) * inp.color;
        }
    }

    if (color.a < 0.5) discard;

    return color;
}