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
    float3 position : Position;
    float4 rotation : Rotation;
    float2 size : Size;
    float2 offset : Offset;
    float4 uv_offset_scale : UvOffsetScale;
    float4 color : Color;
    float4 outline_color : OutlineColor;
    float outline_thickness : OutlineThickness;
    int has_texture : HasTexture;
    int is_ui : IsUI;
    int is_nonscale : IsNonScale;
};

struct VSOutput
{
    nointerpolation float4x4 transform : Transform;
    nointerpolation float4 uv_offset_scale : UvOffsetScale;
    nointerpolation float4 color : Color;
    nointerpolation float4 outline_color : OutlineColor;
    nointerpolation float outline_thickness : OutlineThickness;
    nointerpolation float order : Order;
    nointerpolation int has_texture : HasTexture;
    nointerpolation int is_ui : IsUI;
    nointerpolation int is_nonscale: IsNonScale;
};

struct GSOutput {
    float4 position : SV_Position;
    nointerpolation float4 color : Color;
    nointerpolation float4 outline_color : OutlineColor;
    float2 uv : UV;
    nointerpolation float outline_thickness : OutlineThickness;
    nointerpolation int has_texture : HasTexture;
};

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

    float4x4 transform = float4x4(
        float4(1.0, 0.0, 0.0, inp.position.x),
        float4(0.0, 1.0, 0.0, inp.position.y),
        float4(0.0, 0.0, 1.0, 0.0),
        float4(0.0, 0.0, 0.0, 1.0)
    );

    transform = mul(transform, rotation_matrix);

    float2 offset = -inp.offset * inp.size;

    // translate
    transform[0][3] = transform[0][0] * offset[0] + transform[0][1] * offset[1] + transform[0][2] * 0.0 + transform[0][3];
    transform[1][3] = transform[1][0] * offset[0] + transform[1][1] * offset[1] + transform[1][2] * 0.0 + transform[1][3];
    transform[2][3] = transform[2][0] * offset[0] + transform[2][1] * offset[1] + transform[2][2] * 0.0 + transform[2][3];
    transform[3][3] = transform[3][0] * offset[0] + transform[3][1] * offset[1] + transform[3][2] * 0.0 + transform[3][3];

    //scale
    transform[0][0] = transform[0][0] * inp.size[0];
    transform[1][0] = transform[1][0] * inp.size[0];
    transform[2][0] = transform[2][0] * inp.size[0];
    transform[3][0] = transform[3][0] * inp.size[0];

    transform[0][1] = transform[0][1] * inp.size[1];
    transform[1][1] = transform[1][1] * inp.size[1];
    transform[2][1] = transform[2][1] * inp.size[1];
    transform[3][1] = transform[3][1] * inp.size[1];

	VSOutput outp;
    outp.transform = transform;
    outp.uv_offset_scale = inp.uv_offset_scale;
    outp.color = inp.color;
    outp.outline_color = inp.outline_color;
    outp.outline_thickness = inp.outline_thickness;
    outp.order = inp.position.z;
    outp.has_texture = inp.has_texture;
    outp.is_ui = inp.is_ui;
    outp.is_nonscale = inp.is_nonscale;

	return outp;
}

[maxvertexcount(4)]
void GS(point VSOutput input[1], inout TriangleStream<GSOutput> OutputStream)
{
    float4x4 transform = input[0].transform;
    float4 uv_offset_scale = input[0].uv_offset_scale;
    float4 color = input[0].color;
    float4 outline_color = input[0].outline_color;
    float outline_thickness = input[0].outline_thickness;
    float order = input[0].order;
    int has_texture = input[0].has_texture;
    bool is_ui = input[0].is_ui > 0;
    bool is_nonscale = input[0].is_nonscale > 0;
    
    float4x4 mvp = is_ui ? mul(u_screen_projection, transform) : is_nonscale ? mul(u_nonscale_view_projection, transform) : mul(u_view_projection, transform);

    GSOutput output;
    output.color = color;
    output.has_texture = has_texture;
    output.outline_color = outline_color;
    output.outline_thickness = outline_thickness;

    float2 position = float2(0.0, 0.0);
    output.position = mul(mvp, float4(position, 0.0, 1.0));
    output.position.z = order;
    output.uv = position * uv_offset_scale.zw + uv_offset_scale.xy;
    OutputStream.Append(output);

    position = float2(1.0, 0.0);
    output.position = mul(mvp, float4(position, 0.0, 1.0));
    output.position.z = order;
    output.uv = position * uv_offset_scale.zw + uv_offset_scale.xy;
    OutputStream.Append(output);

    position = float2(0.0, 1.0);
    output.position = mul(mvp, float4(position, 0.0, 1.0));
    output.position.z = order;
    output.uv = position * uv_offset_scale.zw + uv_offset_scale.xy;
    OutputStream.Append(output);

    position = float2(1.0, 1.0);
    output.position = mul(mvp, float4(position, 0.0, 1.0));
    output.position.z = order;
    output.uv = position * uv_offset_scale.zw + uv_offset_scale.xy;
    OutputStream.Append(output);
}

Texture2D Texture : register(t2);
SamplerState Sampler : register(s3);

float4 PS(GSOutput inp) : SV_Target
{
    float4 color = inp.color;

    if (inp.has_texture > 0) {
        if (inp.outline_thickness > 0.0) {
            float outline = Texture.Sample(Sampler, inp.uv + float2(inp.outline_thickness, 0.0)).a;
            outline += Texture.Sample(Sampler, inp.uv + float2(-inp.outline_thickness, 0.0)).a;
            outline += Texture.Sample(Sampler, inp.uv + float2(0.0, inp.outline_thickness)).a;
            outline += Texture.Sample(Sampler, inp.uv + float2(0.0, -inp.outline_thickness)).a;
            outline += Texture.Sample(Sampler, inp.uv + float2(inp.outline_thickness, -inp.outline_thickness)).a;
            outline += Texture.Sample(Sampler, inp.uv + float2(-inp.outline_thickness, inp.outline_thickness)).a;
            outline += Texture.Sample(Sampler, inp.uv + float2(inp.outline_thickness, inp.outline_thickness)).a;
            outline += Texture.Sample(Sampler, inp.uv + float2(-inp.outline_thickness, -inp.outline_thickness)).a;
            outline = min(outline, 1.0);
            float4 c = Texture.Sample(Sampler, inp.uv);
            color = lerp(c, inp.outline_color, outline);
        } else {
            color = Texture.Sample(Sampler, inp.uv) * inp.color;
        }
    }

    if (color.a < 0.5f) discard;

    return color;
};

