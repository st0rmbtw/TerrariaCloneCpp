cbuffer UniformBuffer : register( b1 )
{
    float4x4 u_screen_projection;
    float4x4 u_view_projection;
};

struct VSInput
{
    float3 color : Color;
    float2 position : Position;
    float2 uv : UV;
    int is_ui : IsUI;
};

struct VSOutput
{
    float4 position : SV_Position;
    nointerpolation float3 color : Color;
    float2 uv : UV;
};

VSOutput VS(VSInput inp)
{
    float4x4 mvp = inp.is_ui > 0 ? u_screen_projection : u_view_projection;

	VSOutput outp;
    outp.color = inp.color;
    outp.uv = inp.uv;
    outp.position = mul(mvp, float4(inp.position, 0.0, 1.0));

	return outp;
}

Texture2D Texture : register(t2);
SamplerState Sampler : register(s3);

float4 PS(VSOutput inp) : SV_Target
{
    return float4(inp.color, Texture.Sample(Sampler, inp.uv).r);
};

