cbuffer GlobalUniformBuffer : register( b2 )
{
    float4x4 u_screen_projection;
    float4x4 u_view_projection;
    float4x4 u_nozoom_view_projection;
    float4x4 u_nozoom_projection;
    float4x4 u_transform_matrix;
    float4x4 u_inv_view_proj;
    float2 u_camera_position;
    float2 u_window_size;
};

struct VSInput
{
    float2 position : Position;

    float3 i_position : I_Position;
    float4 i_rotation : I_Rotation;
    float2 i_size : I_Size;
    float2 i_offset : I_Offset;
    float2 i_source_size: I_SourceSize;
    float2 i_output_size: I_OutputSize;
    uint4  i_margin : I_Margin;
    float4 i_uv_offset_scale : I_UvOffsetScale;
    float4 i_color : I_Color;
    int i_flags : I_Flags;
};

struct VSOutput
{
    float4 position : SV_Position;
    nointerpolation float4 color : Color;
    nointerpolation uint4  margin : Margin;
    nointerpolation float2 source_size: SourceSize;
    nointerpolation float2 output_size: OutputSize;
    float2 uv : UV;
};

VSOutput VS(VSInput inp)
{
    const float qxx = inp.i_rotation.x * inp.i_rotation.x;
    const float qyy = inp.i_rotation.y * inp.i_rotation.y;
    const float qzz = inp.i_rotation.z * inp.i_rotation.z;
    const float qxz = inp.i_rotation.x * inp.i_rotation.z;
    const float qxy = inp.i_rotation.x * inp.i_rotation.y;
    const float qyz = inp.i_rotation.y * inp.i_rotation.z;
    const float qwx = inp.i_rotation.w * inp.i_rotation.x;
    const float qwy = inp.i_rotation.w * inp.i_rotation.y;
    const float qwz = inp.i_rotation.w * inp.i_rotation.z;

    const float4x4 rotation_matrix = float4x4(
        float4(1.0 - 2.0 * (qyy + qzz), 2.0 * (qxy - qwz)      , 2.0 * (qxz + qwy)      , 0.0),
        float4(2.0 * (qxy + qwz)      , 1.0 - 2.0 * (qxx + qzz), 2.0 * (qyz - qwx)      , 0.0),
        float4(2.0 * (qxz - qwy)      , 2.0 * (qyz + qwx)      , 1.0 - 2.0 * (qxx + qyy), 0.0),
        float4(0.0                    , 0.0                    , 0.0                    , 1.0)
    );

    float4x4 transform = float4x4(
        float4(1.0, 0.0, 0.0, inp.i_position.x),
        float4(0.0, 1.0, 0.0, inp.i_position.y),
        float4(0.0, 0.0, 1.0, 0.0),
        float4(0.0, 0.0, 0.0, 1.0)
    );

    transform = mul(transform, rotation_matrix);

    const float2 offset = -inp.i_offset * inp.i_size;

    // translate
    transform[0][3] = transform[0][0] * offset[0] + transform[0][1] * offset[1] + transform[0][2] * 0.0 + transform[0][3];
    transform[1][3] = transform[1][0] * offset[0] + transform[1][1] * offset[1] + transform[1][2] * 0.0 + transform[1][3];
    transform[2][3] = transform[2][0] * offset[0] + transform[2][1] * offset[1] + transform[2][2] * 0.0 + transform[2][3];
    transform[3][3] = transform[3][0] * offset[0] + transform[3][1] * offset[1] + transform[3][2] * 0.0 + transform[3][3];

    //scale
    transform[0][0] = transform[0][0] * inp.i_size[0];
    transform[1][0] = transform[1][0] * inp.i_size[0];
    transform[2][0] = transform[2][0] * inp.i_size[0];
    transform[3][0] = transform[3][0] * inp.i_size[0];

    transform[0][1] = transform[0][1] * inp.i_size[1];
    transform[1][1] = transform[1][1] * inp.i_size[1];
    transform[2][1] = transform[2][1] * inp.i_size[1];
    transform[3][1] = transform[3][1] * inp.i_size[1];

    const float4x4 mvp = mul(u_screen_projection, transform);
    const float4 uv_offset_scale = inp.i_uv_offset_scale;

    VSOutput outp;
    outp.position = mul(mvp, float4(inp.position, 0.0, 1.0));
    outp.position.z = 1.0f;
    outp.uv = inp.position * uv_offset_scale.zw + uv_offset_scale.xy;
    outp.color = inp.i_color;
    outp.margin = inp.i_margin;
    outp.source_size = inp.i_source_size;
    outp.output_size = inp.i_output_size;

	return outp;
}

Texture2D Texture : register(t3);
SamplerState Sampler : register(s4);

float map(float value, float in_min, float in_max, float out_min, float out_max) {
    return (value - in_min) / (in_max - in_min) * (out_max - out_min) + out_min;
} 

float process_axis(float coord, float2 source_margin, float2 out_margin) {
    if (coord < out_margin.x) {
        return map(coord, 0.0f, out_margin.x, 0.0f, source_margin.x);
    }
    if (coord < 1.0f - out_margin.y) {
        return map(coord, out_margin.x, 1.0f - out_margin.y, source_margin.x, 1.0f - source_margin.y);
    }
    return map(coord, 1.0f - out_margin.y, 1.0f, 1.0f - source_margin.y, 1.0f);
}

float4 PS(VSOutput inp) : SV_Target
{
    const float2 horizontal_margin = inp.margin.xy;
    const float2 vertical_margin = inp.margin.zw;

    const float2 new_uv = float2(
        process_axis(inp.uv.x,
            horizontal_margin / inp.source_size.xx,
            horizontal_margin / inp.output_size.xx
        ),
        process_axis(inp.uv.y,
            vertical_margin / inp.source_size.yy,
            vertical_margin / inp.output_size.yy
        )
    );

    const float4 color = Texture.Sample(Sampler, new_uv) * inp.color;

    clip(color.a - 0.5);

    return color;
};

