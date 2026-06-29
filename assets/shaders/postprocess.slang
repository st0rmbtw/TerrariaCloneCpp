#include "common.slang"

[vk::binding(2)]
cbuffer GlobalUniformBuffer : register(b2)
{
    GlobalUniforms uniforms;
};

[vk::binding(3)]
cbuffer UniformBuffer : register(b3) {
    float2 uv_scale;
    float2 uv_offset;
}

struct VSInput
{
    float2 position : Position;
    float2 uv : UV;
};

struct VSOutput {
    float4 position : SV_Position;
    float2 uv : UV;
    float2 light_uv : LightUV;
};

[shader("vertex")]
VSOutput VS(VSInput inp)
{
    VSOutput output;
    output.uv = inp.uv;
    output.light_uv = (uv_offset + inp.uv * uv_scale);
    output.position = float4(inp.position, 0.0, 1.0);

	return output;
}

[vk::binding(4)]
Texture2D WorldTexture : register(t4);
[vk::binding(5)]
SamplerState WorldSampler : register(s5);

[vk::binding(6)]
Texture2D LightMapTexture : register(t6);
[vk::binding(7)]
SamplerState LightMapSampler : register(s7);

[vk::binding(8)]
Texture2D LightTexture : register(t8);
[vk::binding(9)]
SamplerState LightSampler : register(s9);

[shader("fragment")]
float4 PS(VSOutput inp) : SV_Target
{
    const float3 light = LightTexture.SampleLevel(LightSampler, inp.light_uv, 0).rgb;
    const float3 lightmap = LightMapTexture.SampleLevel(LightMapSampler, inp.uv, 0).rgb;

    const float4 final_light = float4(max(lightmap, light), 1.0f);
    // const float4 final_light = float4(1.0f);

    const float4 world = WorldTexture.SampleLevel(WorldSampler, inp.uv, 0) * final_light;

    return world;
};

