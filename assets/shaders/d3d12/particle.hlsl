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
    float2 inv_tex_size : InvTexSize;
    float2 tex_size : TexSize;
    float2 i_uv : I_UV;
    float i_depth : I_Depth;
    uint id : SV_InstanceID;
};

struct VSOutput
{
    float4 position : SV_Position;
    float2 uv : UV;
};

static const float2 PARTICLE_SIZE = float2(8.0, 8.0);

RWBuffer<float4> transforms : register(u5);
Buffer<float2> positions : register(t6);
Buffer<float4> rotations : register(t7);
Buffer<float> scales : register(t8);

VSOutput VS(VSInput inp)
{
    float2 position = inp.position;
    float4x4 mvp = float4x4(
        transforms[inp.id * 4 + 0],
        transforms[inp.id * 4 + 1],
        transforms[inp.id * 4 + 2],
        transforms[inp.id * 4 + 3]
    );

    float2 uv = inp.i_uv / inp.tex_size;

    VSOutput output;
    output.position = mul(mvp, float4(position, 0.0, 1.0));
    output.position.z = inp.i_depth / u_max_depth;
    output.uv = uv + position * inp.inv_tex_size;

	return output;
}

[numthreads(64, 1, 1)]
void CSComputeTransform(uint3 thread_id : SV_DispatchThreadID)
{
    const uint id = thread_id.x;

    const float2 position = positions[id];
    const float4 rotation = rotations[id];
    const float scale = scales[id];

    const float qxx = rotation.x * rotation.x;
    const float qyy = rotation.y * rotation.y;
    const float qzz = rotation.z * rotation.z;
    const float qxz = rotation.x * rotation.z;
    const float qxy = rotation.x * rotation.y;
    const float qyz = rotation.y * rotation.z;
    const float qwx = rotation.w * rotation.x;
    const float qwy = rotation.w * rotation.y;
    const float qwz = rotation.w * rotation.z;

    const float4x4 rotation_matrix = float4x4(
        float4(1.0 - 2.0 * (qyy + qzz), 2.0 * (qxy - qwz)      , 2.0 * (qxz + qwy)      , 0.0),
        float4(2.0 * (qxy + qwz)      , 1.0 - 2.0 * (qxx + qzz), 2.0 * (qyz - qwx)      , 0.0),
        float4(2.0 * (qxz - qwy)      , 2.0 * (qyz + qwx)      , 1.0 - 2.0 * (qxx + qyy), 0.0),
        float4(0.0                    , 0.0                    , 0.0                    , 1.0)
    );

    const float2 size = PARTICLE_SIZE * scale;
    const float2 offset = -0.5 * size;

    float4x4 transform = float4x4(
        float4(1.0, 0.0, 0.0, position.x),
        float4(0.0, 1.0, 0.0, position.y),
        float4(0.0, 0.0, 1.0, 0.0),
        float4(0.0, 0.0, 0.0, 1.0)
    );

    transform = mul(transform, rotation_matrix);

    // translate
    transform[0][3] = transform[0][0] * offset[0] + transform[0][1] * offset[1] + transform[0][2] * 0.0 + transform[0][3];
    transform[1][3] = transform[1][0] * offset[0] + transform[1][1] * offset[1] + transform[1][2] * 0.0 + transform[1][3];
    transform[2][3] = transform[2][0] * offset[0] + transform[2][1] * offset[1] + transform[2][2] * 0.0 + transform[2][3];
    transform[3][3] = transform[3][0] * offset[0] + transform[3][1] * offset[1] + transform[3][2] * 0.0 + transform[3][3];

    // scale
    transform[0][0] = transform[0][0] * size[0];
    transform[1][0] = transform[1][0] * size[0];
    transform[2][0] = transform[2][0] * size[0];
    transform[3][0] = transform[3][0] * size[0];

    transform[0][1] = transform[0][1] * size[1];
    transform[1][1] = transform[1][1] * size[1];
    transform[2][1] = transform[2][1] * size[1];
    transform[3][1] = transform[3][1] * size[1];

    const float4x4 mvp = mul(u_view_projection, transform);

    transforms[id * 4 + 0] = mvp[0];
    transforms[id * 4 + 1] = mvp[1];
    transforms[id * 4 + 2] = mvp[2];
    transforms[id * 4 + 3] = mvp[3];
}

Texture2D Texture : register(t3);
SamplerState Sampler : register(s4);

float4 PS(VSOutput inp) : SV_Target
{
    float4 color = Texture.Sample(Sampler, inp.uv);

    clip(color.a - 0.5f);

    return color;
};

