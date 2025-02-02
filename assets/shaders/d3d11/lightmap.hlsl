cbuffer GlobalUniformBuffer : register( b2 )
{
    float4x4 u_screen_projection;
    float4x4 u_view_projection;
    float4x4 u_nozoom_view_projection;
    float4x4 u_nozoom_projection;
    float4x4 u_transform_matrix;
    float4x4 u_inv_view_proj;
    float2 u_camera_position;
    float2 u_window_size;
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

VSOutput VS(VSInput inp)
{
    VSOutput output;
    output.position = mul(u_view_projection, float4(inp.position, 0.0f, 1.0f));
    output.position.z = 1.0f;
    output.uv = inp.uv;

    return output;
}

Texture2D StaticLightMapChunk : register(t3);
SamplerState Sampler : register(s4);

float4 PS(VSOutput inp) : SV_Target
{
    const float4 color = StaticLightMapChunk.Sample(Sampler, inp.uv);
    return float4(color.rgb, 1.0f);
};
