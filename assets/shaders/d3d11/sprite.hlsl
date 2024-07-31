cbuffer UniformBuffer : register( b1 )
{
    float4x4 u_screen_projection;
    float4x4 u_projection;
    float4x4 u_view;
};

struct InputVS
{
    column_major float4x4 transform : Transform;
    float4 uv_offset_scale : UvOffsetScale;
    nointerpolation float4 color : Color;
    nointerpolation uint id : SV_VertexID;
    nointerpolation int has_texture : HasTexture;
    nointerpolation int is_ui : IsUI;
};

struct OutputVS
{
    float4 position : SV_Position;
    nointerpolation float4 color : Color;
    float2 uv : UV;
    nointerpolation int has_texture : HasTexture;
};

OutputVS VS(InputVS inp)
{
	OutputVS outp;

    float2 position = float2(inp.id & 0x1, (inp.id & 0x2) >> 1);

    float4x4 MVP = inp.is_ui > 0 ? mul(u_screen_projection, inp.transform) : mul(mul(u_projection, u_view), inp.transform);
    outp.position = mul(MVP, float4(position, 0.0, 1.0));
    outp.uv = position * inp.uv_offset_scale.zw + inp.uv_offset_scale.xy;
    outp.color = inp.color;
    outp.has_texture = inp.has_texture;

	return outp;
}

Texture2D Texture : register(t2);
SamplerState Sampler : register(s3);

float4 PS(OutputVS inp) : SV_Target
{
    if (inp.has_texture > 0) {
        return Texture.Sample(Sampler, inp.uv) * inp.color;
    } else {
        return inp.color;
    }
};

