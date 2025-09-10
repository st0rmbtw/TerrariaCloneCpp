#include "common.slang"

[vk::binding(2)]
cbuffer GlobalUniformBuffer : register( b2 )
{
    GlobalUniforms uniforms;
};

struct VSInput
{
    float2 position : Position;
    float2 uv : UV;
    float2 world_size : WorldSize;
};

struct VSOutput {
    float4 position : SV_Position;
    float2 uv : UV;
    float2 light_uv : LightUV;
};

inline float2 project_point2(float4x4 mat, float2 p) {
    float2 res = float2(mat[0][0], mat[1][0]) * p.x;
    res = float2(mat[0][1], mat[1][1]) * p.y + res;
    res = float2(mat[0][3], mat[1][3]) + res;
    return res;
}

inline float2 screen_to_world(float2 ndc) {
    return project_point2(uniforms.inv_view_proj, ndc);
}

[shader("vertex")]
VSOutput VS(VSInput inp)
{
    const float2 world_pos = screen_to_world(inp.position);
    const float2 light_uv = world_pos / inp.world_size;

    VSOutput output;
    output.uv = inp.uv;
    output.light_uv = light_uv;
    output.position = float4(inp.position, 0.0, 1.0);

	return output;
}

[vk::binding(3)]
Texture2D BackgroundTexture : register(t3);
[vk::binding(4)]
SamplerState BackgroundTextureSampler : register(s4);

[vk::binding(5)]
Texture2D WorldTexture : register(t5);
[vk::binding(6)]
SamplerState WorldTextureSampler : register(s6);

[vk::binding(7)]
Texture2D LightMap : register(t7);
[vk::binding(8)]
SamplerState LightMapSampler : register(s8);

[vk::binding(9)]
Texture2D Light : register(t9);
[vk::binding(10)]
SamplerState LightSampler : register(s10);

static float4 blend(float4 foreground, float4 background) {
    return foreground * foreground.a + background * (1.0 - foreground.a);
}

[shader("fragment")]
float4 PS(VSOutput inp) : SV_Target
{
    const float3 light = Light.Sample(LightSampler, inp.light_uv).rgb;
    const float3 lightmap = LightMap.Sample(LightMapSampler, inp.uv).rgb;

    const float4 final_light = float4(max(lightmap, light), 1.0f);

    const float4 background = BackgroundTexture.Sample(BackgroundTextureSampler, float2(inp.uv));
    const float4 world = WorldTexture.Sample(WorldTextureSampler, float2(inp.uv)) * final_light;

    return blend(world, background);
};

