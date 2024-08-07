cbuffer UniformBuffer : register( b1 )
{
    float4x4 u_screen_projection;
    float4x4 u_view_projection;
    float4x4 u_nonscale_view_projection;
};

cbuffer OrderBuffer : register( b2 ) {
    float u_tile_order;
    float u_wall_order;
}

struct VSInput
{
    float4 uv_size : UvSize;
    float2 position: Position;
    float2 world_pos: WorldPos;
    nointerpolation uint tile_id: TileId;
    nointerpolation uint tile_type: TileType;
};

struct VSOutput
{
    float4 uv_size : UvSize;
    float4 position : SV_Position;
    float2 world_pos: WorldPos;
    nointerpolation uint tile_id: TileId;
    nointerpolation uint tile_type: TileType;
};

struct GSOutput {
    float4 position : SV_Position;
    float2 uv : UV;
    nointerpolation uint tile_id : TileId;
};

VSOutput VS(VSInput inp)
{
	VSOutput output;
    output.uv_size = inp.uv_size;
    output.world_pos = inp.world_pos;
    output.tile_id = inp.tile_id;
    output.tile_type = inp.tile_type;
    output.position = float4(inp.position * 16.0, 0.0, 1.0);

	return output;
}

static const uint TILE_TYPE_WALL = 1u;

[maxvertexcount(4)]
void GS(point VSOutput input[1], inout TriangleStream<GSOutput> OutputStream)
{
    float2 start_uv = input[0].uv_size.xy;
    float2 tex_size = input[0].uv_size.zw;
    float2 world_pos = input[0].world_pos;
    uint tile_id = input[0].tile_id;
    uint tile_type = input[0].tile_type;
    float4 position = input[0].position;

    float order = u_tile_order;
    float2 size = float2(TILE_SIZE, TILE_SIZE);

    if (tile_type == TILE_TYPE_WALL) {
        order = u_wall_order;
        size = float2(WALL_SIZE, WALL_SIZE);
    }

    float4x4 transform = float4x4(
        float4(1.0, 0.0, 0.0, world_pos.x),
        float4(0.0, 1.0, 0.0, world_pos.y),
        float4(0.0, 0.0, 1.0, 0.0),
        float4(0.0, 0.0, 0.0, 1.0)
    );

    float4x4 mvp = mul(u_view_projection, transform);

    GSOutput output;
    output.tile_id = tile_id;

    output.position = mul(mvp, position);
    output.position.z = order;
    output.uv = start_uv;
    OutputStream.Append(output);

    output.position = mul(mvp, (position + float4(size.x, 0.0, 0.0, 0.0)));
    output.position.z = order;
    output.uv = float2(start_uv.x + tex_size.x, start_uv.y);
    OutputStream.Append(output);

    output.position = mul(mvp, (position + float4(0.0, size.y, 0.0, 0.0)));
    output.position.z = order;
    output.uv = float2(start_uv.x, start_uv.y + tex_size.y);
    OutputStream.Append(output);

    output.position = mul(mvp, (position + float4(size, 0.0, 0.0)));
    output.position.z = order;
    output.uv = start_uv + tex_size;
    OutputStream.Append(output);
}

Texture2DArray TextureArray : register(t3);
SamplerState Sampler : register(s4);

float4 PS(GSOutput inp) : SV_Target
{
    float4 color = TextureArray.Sample(Sampler, float3(inp.uv, float(inp.tile_id)));

    if (color.a < 0.5) discard;

    return color;
};

