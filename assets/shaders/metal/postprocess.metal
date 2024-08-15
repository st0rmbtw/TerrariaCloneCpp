#include <metal_stdlib>

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

float2 project_point2(float4x4 mat, float2 p) {
    float4 res = mat[0] * p.x;
    res = mat[1] * p.y + res;
    res = mat[3] + res;
    return res.xy;
}

float2 screen_to_world(float2 ndc) {
    return project_point2(u_inv_view_proj, ndc);
}

vertex VertexOut VS(
    VertexIn inp [[stage_in]],
    constant Constants& constants [[buffer(1)]]
) {
    float2 world_pos = screen_to_world(inp.position);
    float2 light_uv = world_pos / inp.world_size;

    VSOutput output;
    output.uv = inp.uv;
    output.light_uv = light_uv;
    output.position = float4(inp.position, 0.0, 1.0);
    output.position.z = 0.0;

    return outp;
}

float4 blend(float4 foreground, float4 background) {
    return foreground * foreground.a + background * (1.0 - foreground.a);
}

fragment float4 PS(
    VertexOut inp [[stage_in]],
    texture2d<float> background_texture [[texture(2)]],
    sampler background_texture_sampler [[sampler(3)]],

    texture2d<float> world_texture [[texture(4)]],
    sampler world_texture_sampler [[sampler(5)]],

    texture2d<float> lightmap [[texture(6)]],
    sampler lightmap_sampler [[sampler(7)]]
) {
    float4 light = float4(lightmap.sample(lightmap_sampler, inp.light_uv).rgb, 1.0);

    float4 background = background_texture.sample(background_texture_sampler, inp.uv);
    float4 world = world_texture.sample(world_texture_sampler, inp.uv) * light;

    return blend(world, background);
}