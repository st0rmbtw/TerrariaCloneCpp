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
    float2 world_size : WorldSize;
};

struct VSOutput {
    float4 position : SV_Position;
    float2 uv : UV;
    float2 light_uv : LightUV;
};

inline float2 project_point2(float4x4 mat, float2 p) {
    float4 res = float4(mat[0][0], mat[1][0], mat[2][0], mat[3][0]) * p.x;
    res = float4(mat[0][1], mat[1][1], mat[2][1], mat[3][1]) * p.y + res;
    res = float4(mat[0][3], mat[1][3], mat[2][3], mat[3][3]) + res;
    return res.xy;
}

inline float2 screen_to_world(float2 ndc) {
    return project_point2(u_inv_view_proj, ndc);
}

VSOutput VS(VSInput inp)
{
    const float2 world_pos = screen_to_world(inp.position);
    const float2 light_uv = world_pos / inp.world_size;

	VSOutput output;
    output.uv = inp.uv;
    output.light_uv = light_uv;
    output.position = float4(inp.position, 0.0, 1.0);
    output.position.z = 0.0f;

	return output;
}

Texture2D BackgroundTexture : register(t2); 
SamplerState BackgroundTextureSampler : register(s3);

Texture2D WorldBackgroundTexture : register(t4); 
SamplerState WorldBackgroundTextureSampler : register(s5);

Texture2D WorldTexture : register(t6);
SamplerState WorldTextureSampler : register(s7);

Texture2D LightMap : register(t8);
SamplerState LightMapSampler : register(s9);

float4 blend(float4 foreground, float4 background) {
    return foreground * foreground.a + background * (1.0 - foreground.a);
}

float4 PS(VSOutput inp) : SV_Target
{
    const float4 light = float4(LightMap.Sample(LightMapSampler, float2(inp.light_uv)).rgb, 1.0f);

    const float4 background = BackgroundTexture.Sample(BackgroundTextureSampler, float2(inp.uv));
    const float4 background_world = WorldBackgroundTexture.Sample(WorldBackgroundTextureSampler, float2(inp.uv)) * light;
    const float4 world = WorldTexture.Sample(WorldTextureSampler, float2(inp.uv)) * light;

    return blend(world, blend(background_world, background));
};
