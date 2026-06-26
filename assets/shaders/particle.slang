#include "common.slang"

[vk::binding(2)]
cbuffer GlobalUniformBuffer : register( b2 )
{
    GlobalUniforms uniforms;
};

struct VSInput
{
    float2 position : Position;
    float2 inv_tex_size : InvTexSize;
    float2 tex_size : TexSize;
    float2 i_uv : I_UV;
    float i_depth : I_Depth;
    uint i_id : I_ID;
    uint i_is_world : I_IsWorld;
};

struct VSOutput
{
    float4 position : SV_Position;
    float2 uv : UV;
};

[vk::binding(3)]
Texture2D Texture : register(t3);

[vk::binding(4)]
SamplerState Sampler : register(s4);

[vk::binding(5)]
StructuredBuffer<float4> TransformBuffer : register(t5);

[shader("vertex")]
VSOutput VS(VSInput inp)
{
    const float2 position = inp.position;
    const float4x4 mvp = float4x4(
        TransformBuffer[inp.i_id * 4 + 0],
        TransformBuffer[inp.i_id * 4 + 1],
        TransformBuffer[inp.i_id * 4 + 2],
        TransformBuffer[inp.i_id * 4 + 3]
    );

    const float2 uv = inp.i_uv / inp.tex_size;

    const bool is_world = inp.i_is_world > 0;

    VSOutput output;
    output.position = mul(mvp, float4(position, 0.0, 1.0));
    output.position.z = inp.i_depth;
    output.uv = uv + position * inp.inv_tex_size;

	return output;
}

[shader("fragment")]
float4 PS(VSOutput inp) : SV_Target
{
    float4 color = Texture.Sample(Sampler, inp.uv);

    if (color.a < 0.05f)
        discard;

    return color;
};

