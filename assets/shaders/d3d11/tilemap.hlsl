cbuffer UniformBuffer : register( b0 )
{
    float4x4 u_projection;
    float4x4 u_view;
};

cbuffer TransformBuffer : register( b1 )
{
    float4x4 u_transform;
};

struct VSInput
{
    float2 position: Position;
    float2 atlas_pos: AtlasPos;
    nointerpolation uint tile_id: TileId;
    nointerpolation uint tile_type: TileType;
};

struct VSOutput
{
    float4 position : SV_Position;
    float2 atlas_pos: AtlasPos;
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
    output.atlas_pos = inp.atlas_pos;
    output.tile_id = inp.tile_id;
    output.tile_type = inp.tile_type;
    output.position = float4(inp.position * 16.0, 0.0, 1.0);

	return output;
}

static const float TILE_SIZE = DEF_TILE_SIZE;
static const float WALL_SIZE = DEF_WALL_SIZE;

static const float  TILE_TEXTURE_WIDTH   = DEF_TILE_TEXTURE_WIDTH;
static const float  TILE_TEXTURE_HEIGHT  = DEF_TILE_TEXTURE_HEIGHT;
static const float2 TILE_TEXTURE_PADDING = float2(DEF_TILE_TEXTURE_PADDING, DEF_TILE_TEXTURE_PADDING) / float2(TILE_TEXTURE_WIDTH, TILE_TEXTURE_HEIGHT);

static const float  WALL_TEXTURE_WIDTH   = DEF_WALL_TEXTURE_WIDTH;
static const float  WALL_TEXTURE_HEIGHT  = DEF_WALL_TEXTURE_HEIGHT;
static const float2 WALL_TEXTURE_PADDING = float2(DEF_WALL_TEXTURE_PADDING, DEF_WALL_TEXTURE_PADDING) / float2(WALL_TEXTURE_WIDTH, WALL_TEXTURE_HEIGHT);

static const float2 TILE_TEX_SIZE = float2(TILE_SIZE, TILE_SIZE) / float2(TILE_TEXTURE_WIDTH, TILE_TEXTURE_HEIGHT);
static const float2 WALL_TEX_SIZE = float2(WALL_SIZE, WALL_SIZE) / float2(WALL_TEXTURE_WIDTH, WALL_TEXTURE_HEIGHT);

static const uint TILE_TYPE_WALL = 1u;

[maxvertexcount(4)]
void GS(point VSOutput input[1], inout TriangleStream<GSOutput> OutputStream)
{
    float2 atlas_pos = input[0].atlas_pos;
    uint tile_id = input[0].tile_id;
    uint tile_type = input[0].tile_type;
    float4 position = input[0].position;

    float2 size = float2(TILE_SIZE, TILE_SIZE);
    float2 tex_size = TILE_TEX_SIZE;
    float2 padding = TILE_TEXTURE_PADDING;

    if (tile_type == TILE_TYPE_WALL) {
        size = float2(WALL_SIZE, WALL_SIZE);
        tex_size = WALL_TEX_SIZE;
        padding = WALL_TEXTURE_PADDING;
    }

    float2 start_uv = atlas_pos * (tex_size + padding);

    float4x4 mvp = mul(mul(u_projection, u_view), u_transform);

    GSOutput output;
    output.tile_id = tile_id;

    output.position = mul(mvp, position);
    output.uv = start_uv;
    OutputStream.Append(output);

    output.position = mul(mvp, (position + float4(size.x, 0.0, 0.0, 0.0)));
    output.uv = float2(start_uv.x + tex_size.x, start_uv.y);
    OutputStream.Append(output);

    output.position = mul(mvp, (position + float4(0.0, size.y, 0.0, 0.0)));
    output.uv = float2(start_uv.x, start_uv.y + tex_size.y);
    OutputStream.Append(output);

    output.position = mul(mvp, (position + float4(size, 0.0, 0.0)));
    output.uv = float2(start_uv.x + tex_size.x, start_uv.y + tex_size.y);
    OutputStream.Append(output);
}

Texture2DArray TextureArray : register(t2);
SamplerState Sampler : register(s3);

float4 PS(GSOutput inp) : SV_Target
{
    return TextureArray.Sample(Sampler, float3(inp.uv, float(inp.tile_id)));
};

