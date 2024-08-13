cbuffer GlobalUniformBuffer : register( b1 )
{
    float4x4 u_screen_projection;
    float4x4 u_view_projection;
    float4x4 u_nonscale_view_projection;
    float4x4 u_nonscale_projection;
    float4x4 u_transform_matrix;
    float4x4 u_inv_view_proj;
    float2 u_camera_position;
    float2 u_window_size;
    float u_max_depth;
};

struct VSInput
{
    float2 position : Position;
    float2 uv : UV;
    float2 size : Size;
    float2 tex_size : TexSize;
    float2 speed : Speed;
    int nonscale : NonScale;
};

struct VSOutput
{
    float4 position : SV_Position;
    float2 uv : UV;
    nointerpolation float2 size : Size;
    nointerpolation float2 tex_size : TexSize;
    nointerpolation float2 speed : Speed;
    nointerpolation int nonscale : NonScale;
    nointerpolation float2 offset : Offset;
};

// Bidirectional mod
float fmodb(float a, float b) {
    float r = fmod(a, b);
    return r < 0.0 ? r + b : r;
}

Texture2D Texture : register(t2);
SamplerState Sampler : register(s3);

float4 scroll(
    float2 speed,
    float2 uv,
    float2 offset,
    float2 tex_size,
    float2 size,
    bool nonscale
) {
    const float2 new_offset = float2(-offset.x, offset.y);

    if (!nonscale) {
        speed.x = speed.x / (size.x / tex_size.x);
    }

    float2 new_uv = uv - (new_offset * float2(speed.x, 0.0f));
    if (!nonscale) {
        new_uv = new_uv * (size / tex_size);
    }

    new_uv.x = fmodb(new_uv.x, 1.0);
    new_uv.y = fmodb(new_uv.y, 1.0);

    return Texture.Sample(Sampler, new_uv);
}

VSOutput VS(VSInput inp)
{
    const float4x4 view_proj = inp.nonscale > 0 ? u_nonscale_view_projection : u_view_projection;
    const float4x4 proj_model = mul(u_nonscale_projection, u_transform_matrix);

    const float2 offset = mul(proj_model, float4(u_camera_position.x, u_camera_position.y, 0.0, 1.0)).xy;

	VSOutput outp;
    outp.position = mul(view_proj, float4(inp.position.x, inp.position.y, 0.0, 1.0));
    outp.position.z = 0.0;
    outp.uv = inp.uv;
    outp.tex_size = inp.tex_size;
    outp.size = inp.size;
    outp.speed = inp.speed;
    outp.nonscale = inp.nonscale;
    outp.offset = offset;

	return outp;
}

float4 PS(VSOutput inp) : SV_Target
{
    const float4 color = scroll(inp.speed, inp.uv, inp.offset, inp.tex_size, inp.size, inp.nonscale > 0);

    clip(color.a - 0.05f);

    return color;
};

