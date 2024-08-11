cbuffer GlobalUniformBuffer : register( b1 )
{
    float4x4 u_screen_projection;
    float4x4 u_view_projection;
    float4x4 u_nonscale_view_projection;
    float4x4 u_nonscale_projection;
    float4x4 u_transform_matrix;
    float2 u_camera_position;
    float2 u_window_size;
    float u_max_depth;
};

struct VSInput
{
    float2 position : Position;
    
    float3 i_color : I_Color;
    float3 i_position : I_Position;
    float2 i_size : I_Size;
    float2 i_tex_size : I_TexSize;
    float2 i_uv : I_UV;
    int i_is_ui : I_IsUI;
};

struct VSOutput
{
    float4 position : SV_Position;
    nointerpolation float3 color : Color;
    float2 uv : UV;
};

VSOutput VS(VSInput inp)
{
    const float4x4 mvp = inp.i_is_ui > 0 ? u_screen_projection : u_view_projection;

    const float2 position = inp.i_position.xy + inp.position * inp.i_size;
    const float2 uv = inp.i_uv + inp.position * inp.i_tex_size;

	VSOutput outp;
    outp.color = inp.i_color;
    outp.uv = uv;
    outp.position = mul(mvp, float4(position, 0.0, 1.0));
    outp.position.z = inp.i_position.z / u_max_depth;

	return outp;
}

Texture2D Texture : register(t2);
SamplerState Sampler : register(s3);

float4 PS(VSOutput inp) : SV_Target
{
    const float4 color = float4(inp.color, Texture.Sample(Sampler, inp.uv).r);

    clip(color.a - 0.05f);

    return color;
};

