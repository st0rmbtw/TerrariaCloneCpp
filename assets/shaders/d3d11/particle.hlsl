cbuffer GlobalUniformBuffer : register( b1 )
{
    float4x4 u_screen_projection;
    float4x4 u_view_projection;
    float4x4 u_nonscale_view_projection;
    float4x4 u_nonscale_projection;
    float4x4 u_transform_matrix;
    float2 u_camera_position;
    float2 u_window_size;
};

struct VSInput
{
    float2 position : Position;
    float4 rotation : Rotation;
    float2 uv : UV;
    float2 tex_size : TexSize;
    float scale : Scale;
};

struct VSOutput
{
    nointerpolation float4x4 transform : Transform;
    nointerpolation float2 uv : UV;
    nointerpolation float2 tex_size : TexSize;
};

struct GSOutput {
    float4 position : SV_Position;
    float2 uv : UV;
};

static const float2 PARTICLE_SIZE = float2(8.0, 8.0);

VSOutput VS(VSInput inp)
{
    float qxx = inp.rotation.x * inp.rotation.x;
    float qyy = inp.rotation.y * inp.rotation.y;
    float qzz = inp.rotation.z * inp.rotation.z;
    float qxz = inp.rotation.x * inp.rotation.z;
    float qxy = inp.rotation.x * inp.rotation.y;
    float qyz = inp.rotation.y * inp.rotation.z;
    float qwx = inp.rotation.w * inp.rotation.x;
    float qwy = inp.rotation.w * inp.rotation.y;
    float qwz = inp.rotation.w * inp.rotation.z;

    float4x4 rotation_matrix = float4x4(
        float4(1.0 - 2.0 * (qyy + qzz), 2.0 * (qxy - qwz)      , 2.0 * (qxz + qwy)      , 0.0),
        float4(2.0 * (qxy + qwz)      , 1.0 - 2.0 * (qxx + qzz), 2.0 * (qyz - qwx)      , 0.0),
        float4(2.0 * (qxz - qwy)      , 2.0 * (qyz + qwx)      , 1.0 - 2.0 * (qxx + qyy), 0.0),
        float4(0.0                    , 0.0                    , 0.0                    , 1.0)
    );

    float2 size = PARTICLE_SIZE * inp.scale;
    float2 offset = -0.5 * size;

    float4x4 transform = float4x4(
        float4(1.0, 0.0, 0.0, inp.position.x),
        float4(0.0, 1.0, 0.0, inp.position.y),
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

	VSOutput outp;
    outp.transform = transform;
    outp.uv = inp.uv / inp.tex_size;
    outp.tex_size = PARTICLE_SIZE / inp.tex_size;

	return outp;
}

[maxvertexcount(4)]
void GS(point VSOutput input[1], inout TriangleStream<GSOutput> OutputStream)
{
    float4x4 transform = input[0].transform;
    float2 uv = input[0].uv;
    float2 tex_size = input[0].tex_size;
    
    float4x4 mvp = mul(u_view_projection, transform);

    GSOutput output;

    float2 position = float2(0.0, 0.0);
    output.position = mul(mvp, float4(position, 0.0, 1.0));
    output.position.z = 1.0f;
    output.uv = uv;
    OutputStream.Append(output);

    position = float2(1.0, 0.0);
    output.position = mul(mvp, float4(position, 0.0, 1.0));
    output.position.z = 1.0f;
    output.uv = uv + float2(tex_size.x, 0.0);
    OutputStream.Append(output);

    position = float2(0.0, 1.0);
    output.position = mul(mvp, float4(position, 0.0, 1.0));
    output.position.z = 1.0f;
    output.uv = uv + float2(0.0, tex_size.y);
    OutputStream.Append(output);

    position = float2(1.0, 1.0);
    output.position = mul(mvp, float4(position, 0.0, 1.0));
    output.position.z = 1.0f;
    output.uv = uv + tex_size;
    OutputStream.Append(output);
}

Texture2D Texture : register(t2);
SamplerState Sampler : register(s3);

float4 PS(GSOutput inp) : SV_Target
{
    float4 color = Texture.Sample(Sampler, inp.uv);

    if (color.a < 0.5f) discard;

    return color;
};

