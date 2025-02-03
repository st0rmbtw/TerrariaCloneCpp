cbuffer GlobalUniformBuffer : register( b2 )
{
    float4x4 u_screen_projection;
    float4x4 u_view_projection;
    float4x4 u_nonscale_view_projection;
    float4x4 u_nonscale_projection;
    float4x4 u_transform_matrix;
    float4x4 u_inv_view_proj;
    float2 u_camera_position;
    float2 u_window_size;
};

struct VSInput
{
    float2 position : Position;
    
    float3 i_color : I_Color;
    float2 i_position : I_Position;
    float2 i_size : I_Size;
    float2 i_tex_size : I_TexSize;
    float2 i_uv : I_UV;
};

struct VSOutput
{
    float4 position : SV_Position;
    nointerpolation float3 color : Color;
    nointerpolation float2 tex_size : TexSize;
    float2 uv : UV;
};

VSOutput VS(VSInput inp)
{
    const float4x4 mvp = u_screen_projection;

    const float2 position = inp.i_position + inp.position * inp.i_size;
    const float2 uv = inp.i_uv + inp.position * inp.i_tex_size;

    VSOutput outp;
    outp.color = inp.i_color;
    outp.uv = uv;
    outp.tex_size = inp.i_tex_size;
    outp.position = mul(mvp, float4(position, 0.0, 1.0));
    outp.position.z = 1.0f;

	return outp;
}

Texture2D Texture : register(t3);
SamplerState Sampler : register(s4);

static const float GLYPH_CENTER = 0.5;

float4 PS(VSOutput inp) : SV_Target
{
    const float outline = 0.2;

    const float dist = Texture.Sample(Sampler, inp.uv).r;
    const float width = fwidth(dist);
    const float alpha = smoothstep(GLYPH_CENTER - outline - width, GLYPH_CENTER - outline + width, abs(dist));
    // float4 color = float4(inp.color, alpha);

    const float mu = smoothstep(0.5-width, 0.5+width, abs(dist));
    const float3 rgb = lerp(float3(0.0, 0.0, 0.0), inp.color, mu);
    const float4 color = float4(rgb, alpha);

    clip(color.a - 0.05f);

    return color;
};

