cbuffer UniformBuffer : register( b0 )
{
    float4x4 u_view_projection;
};

struct InputVS
{
    column_major float4x4 transform : Transform;
    float4 uv_offset_scale : UvOffsetScale;
    nointerpolation float4 color : Color;
    uint id : SV_VertexID;
};

struct OutputVS
{
    float4 position : SV_Position;
    nointerpolation float4 color : Color;
    float2 uv : UV;
};

OutputVS VS(InputVS inp)
{
	OutputVS outp;

    float2 position = float2(inp.id & 0x1, (inp.id & 0x2) >> 1);

    float4x4 MVP = mul(u_view_projection, inp.transform);
    outp.position = mul(MVP, float4(position, 0.0, 1.0));
    outp.uv = position * inp.uv_offset_scale.zw + inp.uv_offset_scale.xy;
    outp.color = inp.color;

	return outp;
}

float4 PS(OutputVS inp) : SV_Target
{
    // return GetTextureStateByIndex(inp.texture_index).Sample(inp.texture_index == 0 ? SamplerLinear : SamplerNearest, inp.uv);
    return inp.color;
};

