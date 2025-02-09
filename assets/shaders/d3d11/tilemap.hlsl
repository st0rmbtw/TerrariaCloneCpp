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

cbuffer DepthBuffer : register( b3 ) {
    float u_tile_depth;
    float u_wall_depth;
};

struct VSInput
{
    float2 position : Position;
    float2 wall_tex_size : WallTexSize;
    float2 tile_tex_size : TileTexSize;
    float2 wall_padding : WallPadding;
    float2 tile_padding : TilePadding;

    float2 i_position: I_Position;
    float2 i_atlas_pos : I_AtlasPos;
    float2 i_world_pos: I_WorldPos;
    nointerpolation uint i_tile_data: I_TileData;
};

struct VSOutput {
    float4 position : SV_Position;
    float2 uv : UV;
    nointerpolation uint tile_id : TileId;
};

static const uint TILE_TYPE_WALL = 1u;

VSOutput VS(VSInput inp)
{
    const float2 world_pos = inp.i_world_pos;
    const uint tile_data = inp.i_tile_data;

    // Extract last 6 bits
    const uint tile_type = tile_data & 0x3f;
    // Extract other 10 bits
    const uint tile_id = (tile_data >> 6) & 0x3ff;

    float depth = u_tile_depth;
    float2 size = float2(TILE_SIZE, TILE_SIZE);
    float2 tex_size = size / inp.tile_tex_size;
    float2 start_uv = inp.i_atlas_pos * (tex_size + inp.tile_padding);
    float2 tex_dims = inp.tile_tex_size;
    float2 offset = float2(0.0, 0.0);

    if (tile_type == TILE_TYPE_WALL) {
        depth = u_wall_depth;
        size = float2(WALL_SIZE, WALL_SIZE);
        tex_size = size / inp.wall_tex_size;
        start_uv = inp.i_atlas_pos * (tex_size + inp.wall_padding);
        tex_dims = inp.wall_tex_size;
        offset = float2(-TILE_SIZE * 0.5, -TILE_SIZE * 0.5);
    }

    const float4x4 transform = float4x4(
        float4(1.0, 0.0, 0.0, world_pos.x),
        float4(0.0, 1.0, 0.0, world_pos.y),
        float4(0.0, 0.0, 1.0, 0.0),
        float4(0.0, 0.0, 0.0, 1.0)
    );

    const float4x4 mvp = mul(u_view_projection, transform);
    const float2 position = inp.i_position * 16.0 + inp.position * size + offset;
    const float2 uv = start_uv + inp.position * tex_size;

    const float2 pixel_offset = float2(0.1 / tex_dims.x, 0.1 / tex_dims.y);

    VSOutput output;
    output.uv = uv + pixel_offset * (float2(1.0, 1.0) - inp.position * 2.0);
    output.tile_id = tile_id;
    output.position = mul(mvp, float4(position, 0.0, 1.0));
    output.position.z = depth;

	return output;
}

Texture2DArray TextureArray : register(t4);
SamplerState Sampler : register(s5);

float4 PS(VSOutput inp) : SV_Target
{
    const float4 color = TextureArray.Sample(Sampler, float3(inp.uv, float(inp.tile_id)));

    clip(color.a - 0.5);

    return float4(color.rgb, 1.0f);
};

