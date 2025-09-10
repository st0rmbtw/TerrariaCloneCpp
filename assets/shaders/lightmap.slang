#include "common.slang"

[vk::binding(2)]
cbuffer GlobalUniformBuffer : register( b2 )
{
    GlobalUniforms uniforms;
};

struct VSInput
{
    float2 position : Position;
    float2 uv: UV;
};

struct VSOutput {
    float4 position : SV_Position;
    float2 uv : UV;
};

[shader("vertex")]
VSOutput VS(VSInput inp)
{
    VSOutput output;
    output.position = mul(uniforms.view_projection, float4(inp.position, 0.0f, 1.0f));
    output.position.z = 1.0f;
    output.uv = inp.uv;
    return output;
}

[vk::binding(3)]
Texture2D StaticLightMapChunk : register(t3);

[vk::binding(4)]
SamplerState Sampler : register(s4);

[shader("fragment")]
float4 PS(VSOutput inp) : SV_Target
{
    const float4 color = StaticLightMapChunk.Sample(Sampler, inp.uv);
    return float4(color.rgb, 1.0f);
};
