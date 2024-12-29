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
    float u_max_depth;
    float u_max_world_depth;
};

struct VSInput
{
    float2 position : Position;
    float2 texture_size : TextureSize;

    float2 i_position : I_Position;
    float2 i_size : I_Size;
    float2 i_tex_size : I_TexSize;
    float2 i_speed : I_Speed;
    uint i_id : I_ID;
    int i_flags : I_Flags;
};

struct VSOutput
{
    float4 position : SV_Position;
    nointerpolation float2 texture_size : TextureSize;
    float2 uv : UV;
    nointerpolation float2 size : Size;
    nointerpolation float2 tex_size : TexSize;
    nointerpolation float2 speed : Speed;
    nointerpolation uint id : I_ID;
    nointerpolation float2 offset : Offset;
    nointerpolation int nonscale : NonScale;
};

// Bidirectional mod
float fmodb(float a, float b) {
    float r = fmod(a, b);
    return r < 0.0 ? r + b : r;
}

Texture2DArray Texture : register(t3);
SamplerState Sampler : register(s4);

float4 scroll(
    float2 speed,
    float2 uv,
    float2 offset,
    float2 tex_size,
    float2 texture_size,
    float2 size,
    uint id,
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

    if (id == 0) {
        float x = new_uv.x;
        new_uv.x = new_uv.y;
        new_uv.y = x;
    }

    new_uv = new_uv * tex_size / texture_size;
    
    return Texture.Sample(Sampler, float3(new_uv, float(id)));
}

static const int IGNORE_CAMERA_ZOOM_FLAG = 1 << 0;

VSOutput VS(VSInput inp)
{
    const int flags = inp.i_flags;
    const bool ignore_camera_zoom = (flags & IGNORE_CAMERA_ZOOM_FLAG) == IGNORE_CAMERA_ZOOM_FLAG;

    const float4x4 view_proj = ignore_camera_zoom ? u_nonscale_view_projection : u_view_projection;
    const float4x4 proj_model = mul(u_nonscale_projection, u_transform_matrix);

    const float2 offset = mul(proj_model, float4(u_camera_position.x, u_camera_position.y, 0.0, 1.0)).xy;

    const float2 position = inp.i_position.xy + inp.i_size * inp.position;

    VSOutput outp;
    outp.position = mul(view_proj, float4(position.x, position.y, 0.0, 1.0));
    outp.position.z = 0.0;

    outp.texture_size = inp.texture_size;
    outp.uv = inp.position;
    outp.tex_size = inp.i_tex_size;
    outp.size = inp.i_size;
    outp.speed = inp.i_speed;
    outp.nonscale = ignore_camera_zoom;
    outp.id = inp.i_id;
    outp.offset = offset;

	return outp;
}

float4 PS(VSOutput inp) : SV_Target
{
    const float4 color = scroll(inp.speed, inp.uv, inp.offset, inp.tex_size, inp.texture_size, inp.size, inp.id, inp.nonscale > 0);

    clip(color.a - 0.05f);

    return color;
};

