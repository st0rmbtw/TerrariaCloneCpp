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
};

struct VertexIn
{
    float2 position   [[attribute(0)]];
    float2 uv         [[attribute(1)]];
    float2 world_size [[attribute(2)]];
};

struct VertexOut
{
    float4 position [[position]];
    float2 uv;
    float2 light_uv;
};

float2 screen_to_world(float4x4 mat, float2 p) {
    float4 res = mat[0] * p.x;
    res = mat[1] * p.y + res;
    res = mat[3] + res;
    return res.xy;
}

vertex VertexOut VS(
    VertexIn inp [[stage_in]],
    constant Constants& constants [[buffer(2)]]
) {
    const float2 world_pos = screen_to_world(constants.inv_view_proj, inp.position);
    const float2 light_uv = world_pos / inp.world_size;

    VertexOut outp;
    outp.uv = inp.uv;
    outp.light_uv = light_uv;
    outp.position = float4(inp.position, 0.0, 1.0);
    outp.position.z = 0.0;

    return outp;
}

float4 blend(float4 foreground, float4 background) {
    return foreground * foreground.a + background * (1.0 - foreground.a);
}

fragment float4 PS(
    VertexOut inp [[stage_in]],
    texture2d<float> background_texture [[texture(3)]],
    sampler background_texture_sampler [[sampler(4)]],

    texture2d<float> world_texture [[texture(5)]],
    sampler world_texture_sampler [[sampler(6)]],

    texture2d<float> lightmap_texture [[texture(7)]],
    sampler lightmap_sampler [[sampler(8)]],

    texture2d<float> light_texture [[texture(9)]],
    sampler light_sampler [[sampler(10)]]
) {
    const float3 light = light_texture.sample(light_sampler, inp.light_uv).rgb;
    const float3 lightmap = lightmap_texture.sample(lightmap_sampler, inp.uv).rgb;

    const float4 final_light = float4(max(lightmap, light), 1.0);

    const float4 background = background_texture.sample(background_texture_sampler, inp.uv);
    const float4 world = world_texture.sample(world_texture_sampler, inp.uv) * final_light;

    return blend(world, background);
}