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
};

struct VertexIn
{
    float2 position            [[attribute(0)]];

    float3 i_position          [[attribute(1)]];
    float4 i_rotation          [[attribute(2)]];
    float2 i_size              [[attribute(3)]];
    float2 i_offset            [[attribute(4)]];
    float4 i_uv_offset_scale   [[attribute(5)]];
    float4 i_color             [[attribute(6)]];
    float4 i_outline_color     [[attribute(7)]];
    float  i_outline_thickness [[attribute(8)]];
    int    i_has_texture       [[attribute(9)]];
    int    i_is_ui             [[attribute(10)]];
    int    i_is_nonscale       [[attribute(11)]];
};

struct VertexOut
{
    float4 position          [[position]];
    float2 uv;
    float4 color             [[flat]];
    float4 outline_color     [[flat]];
    float  outline_thickness [[flat]];
    int    has_texture       [[flat]];
};

vertex VertexOut VS(
    VertexIn inp [[stage_in]],
    constant Constants& constants [[buffer(1)]]
) {
    float qxx = inp.i_rotation.x * inp.i_rotation.x;
    float qyy = inp.i_rotation.y * inp.i_rotation.y;
    float qzz = inp.i_rotation.z * inp.i_rotation.z;
    float qxz = inp.i_rotation.x * inp.i_rotation.z;
    float qxy = inp.i_rotation.x * inp.i_rotation.y;
    float qyz = inp.i_rotation.y * inp.i_rotation.z;
    float qwx = inp.i_rotation.w * inp.i_rotation.x;
    float qwy = inp.i_rotation.w * inp.i_rotation.y;
    float qwz = inp.i_rotation.w * inp.i_rotation.z;

    float4x4 rotation_matrix = float4x4(
        float4(1.0 - 2.0 * (qyy + qzz), 2.0 * (qxy + qwz), 2.0 * (qxz - qwy), 0.0),
        float4(2.0 * (qxy - qwz), 1.0 - 2.0 * (qxx +  qzz), 2.0 * (qyz + qwx), 0.0),
        float4(2.0 * (qxz + qwy), 2.0 * (qyz - qwx), 1.0 - 2.0 * (qxx +  qyy), 0.0),
        float4(0.0, 0.0, 0.0, 1.0)
    );

    float4x4 transform = float4x4(
        float4(1.0, 0.0, 0.0, 0.0),
        float4(0.0, 1.0, 0.0, 0.0),
        float4(0.0, 0.0, 1.0, 0.0),
        float4(inp.position.x, inp.position.y, 0.0, 1.0)
    );

    // transform = transform * rotation_matrix;

    float2 offset = -inp.i_offset * inp.i_size;

    // translate
    transform[3] = transform[0] * offset[0] + transform[1] * offset[1] + transform[2] * 0.0 + transform[3];

    // scale
    transform[0] = transform[0] * inp.i_size[0];
    transform[1] = transform[1] * inp.i_size[1];

    float4x4 mvp = inp.i_is_ui ? constants.screen_projection * transform : inp.i_is_nonscale ? constants.nonscale_view_projection * transform : constants.view_projection * transform;
    float4 uv_offset_scale = inp.i_uv_offset_scale;
    float2 position = inp.position;
    float order = inp.i_position.z / constants.max_depth;

    VertexOut outp;
    outp.position = mvp * float4(position, 0.0, 1.0);
    outp.position.z = order;
    outp.uv = position * uv_offset_scale.zw + uv_offset_scale.xy;
    outp.color = inp.i_color;
    outp.outline_color = inp.i_outline_color;
    outp.outline_thickness = inp.i_outline_thickness;
    outp.has_texture = inp.i_has_texture;

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

    if (color.a < 0.5) discard_fragment();

    return color;
}