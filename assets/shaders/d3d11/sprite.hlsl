cbuffer UniformBuffer : register( b1 )
{
    float4x4 u_screen_projection;
    float4x4 u_view_projection;
};

struct VSInput
{
    float4x4 transform : Transform;
    float4 uv_offset_scale : UvOffsetScale;
    float4 color : Color;
    float4 outline_color : OutlineColor;
    float outline_thickness : OutlineThickness;
    int has_texture : HasTexture;
    int is_ui : IsUI;
};

struct VSOutput
{
    nointerpolation float4x4 transform : Transform;
    nointerpolation float4 uv_offset_scale : UvOffsetScale;
    nointerpolation float4 color : Color;
    nointerpolation float4 outline_color : OutlineColor;
    nointerpolation float outline_thickness : OutlineThickness;
    nointerpolation int has_texture : HasTexture;
    nointerpolation float is_ui : IsUI;
};

struct GSOutput {
    float4 position : SV_Position;
    nointerpolation float4 color : Color;
    nointerpolation float4 outline_color : OutlineColor;
    float2 uv : UV;
    nointerpolation float outline_thickness : OutlineThickness;
    nointerpolation int has_texture : HasTexture;
};

VSOutput VS(VSInput inp)
{
	VSOutput outp;
    outp.transform = inp.transform;
    outp.uv_offset_scale = inp.uv_offset_scale;
    outp.color = inp.color;
    outp.outline_color = inp.outline_color;
    outp.outline_thickness = inp.outline_thickness;
    outp.has_texture = inp.has_texture;
    outp.is_ui = inp.is_ui;

	return outp;
}

[maxvertexcount(4)]
void GS(point VSOutput input[1], inout TriangleStream<GSOutput> OutputStream)
{
    float4x4 transform = input[0].transform;
    float4 uv_offset_scale = input[0].uv_offset_scale;
    float4 color = input[0].color;
    float4 outline_color = input[0].outline_color;
    float outline_thickness = input[0].outline_thickness;
    float is_ui = input[0].is_ui;
    int has_texture = input[0].has_texture;
    
    float4x4 mvp = is_ui > 0 ? mul(u_screen_projection, transform) : mul(u_view_projection, transform);

    GSOutput output;
    output.color = color;
    output.has_texture = has_texture;
    output.outline_color = outline_color;
    output.outline_thickness = outline_thickness;

    float2 position = float2(0.0, 0.0);
    output.position = mul(mvp, float4(position, 0.0, 1.0));
    output.uv = position * uv_offset_scale.zw + uv_offset_scale.xy;
    OutputStream.Append(output);

    position = float2(1.0, 0.0);
    output.position = mul(mvp, float4(position, 0.0, 1.0));
    output.uv = position * uv_offset_scale.zw + uv_offset_scale.xy;
    OutputStream.Append(output);

    position = float2(0.0, 1.0);
    output.position = mul(mvp, float4(position, 0.0, 1.0));
    output.uv = position * uv_offset_scale.zw + uv_offset_scale.xy;
    OutputStream.Append(output);

    position = float2(1.0, 1.0);
    output.position = mul(mvp, float4(position, 0.0, 1.0));
    output.uv = position * uv_offset_scale.zw + uv_offset_scale.xy;
    OutputStream.Append(output);
}

Texture2D Texture : register(t2);
SamplerState Sampler : register(s3);

float4 PS(GSOutput inp) : SV_Target
{
    if (inp.has_texture > 0) {
        if (inp.outline_thickness > 0.0) {
            float outline = Texture.Sample(Sampler, inp.uv + float2(inp.outline_thickness, 0.0)).a;
            outline += Texture.Sample(Sampler, inp.uv + float2(-inp.outline_thickness, 0.0)).a;
            outline += Texture.Sample(Sampler, inp.uv + float2(0.0, inp.outline_thickness)).a;
            outline += Texture.Sample(Sampler, inp.uv + float2(0.0, -inp.outline_thickness)).a;
            outline += Texture.Sample(Sampler, inp.uv + float2(inp.outline_thickness, -inp.outline_thickness)).a;
            outline += Texture.Sample(Sampler, inp.uv + float2(-inp.outline_thickness, inp.outline_thickness)).a;
            outline += Texture.Sample(Sampler, inp.uv + float2(inp.outline_thickness, inp.outline_thickness)).a;
            outline += Texture.Sample(Sampler, inp.uv + float2(-inp.outline_thickness, -inp.outline_thickness)).a;
            outline = min(outline, 1.0);
            float4 c = Texture.Sample(Sampler, inp.uv);
            return lerp(c, inp.outline_color, outline);
        } else {
            return Texture.Sample(Sampler, inp.uv) * inp.color;
        }
    } else {
        return inp.color;
    }
};

