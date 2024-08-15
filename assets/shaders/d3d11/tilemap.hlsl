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
    float u_max_world_depth;
};

cbuffer DepthBuffer : register( b2 ) {
    float u_tile_depth;
    float u_wall_depth;
};

struct VSInput
{
    float2 position: Position;
    float2 wall_tex_size : WallTexSize;
    float2 tile_tex_size : TileTexSize;
    float2 wall_padding : WallPadding;
    float2 tile_padding : TilePadding;

    float2 i_position: I_Position;
    float2 i_atlas_pos : I_AtlasPos;
    float2 i_world_pos: I_WorldPos;
    nointerpolation uint i_tile_id: I_TileId;
    nointerpolation uint i_tile_type: I_TileType;
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
    const uint tile_id = inp.i_tile_id;
    const uint tile_type = inp.i_tile_type;

    float order = u_tile_depth;
    float2 size = float2(TILE_SIZE, TILE_SIZE);
    float2 start_uv = inp.i_atlas_pos * (inp.tile_tex_size + inp.tile_padding);
    float2 tex_size = inp.tile_tex_size;

    if (tile_type == TILE_TYPE_WALL) {
        order = u_wall_depth;
        size = float2(WALL_SIZE, WALL_SIZE);
        start_uv = inp.i_atlas_pos * (inp.wall_tex_size + inp.wall_padding);
        tex_size = inp.wall_tex_size;
    }

    order /= u_max_world_depth;

    const float4x4 transform = float4x4(
        float4(1.0, 0.0, 0.0, world_pos.x),
        float4(0.0, 1.0, 0.0, world_pos.y),
        float4(0.0, 0.0, 1.0, 0.0),
        float4(0.0, 0.0, 0.0, 1.0)
    );

    const float4x4 mvp = mul(u_view_projection, transform);
    const float2 position = inp.i_position * 16.0 + inp.position * size;

    VSOutput output;
    output.uv = start_uv + inp.position * tex_size;
    output.tile_id = inp.i_tile_id;
    output.position = mul(mvp, float4(position, 0.0, 1.0));
    output.position.z = order;

	return output;
}

Texture2DArray TextureArray : register(t3);
SamplerState Sampler : register(s4);

float4 PS(VSOutput inp) : SV_Target
{
    const float4 color = TextureArray.Sample(Sampler, float3(inp.uv, float(inp.tile_id)));

    clip(color.a - 0.5);

    return float4(color.rgb, 1.0f);
};

