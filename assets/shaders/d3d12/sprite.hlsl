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
    float u_max_depth;
    float u_max_world_depth;
};

struct VSInput
{
    float2 position : Position;

    float3 i_position : I_Position;
    float4 i_rotation : I_Rotation;
    float2 i_size : I_Size;
    float2 i_offset : I_Offset;
    float4 i_uv_offset_scale : I_UvOffsetScale;
    float4 i_color : I_Color;
    float4 i_outline_color : I_OutlineColor;
    float i_outline_thickness : I_OutlineThickness;
    int i_flags : I_Flags;
};

struct VSOutput
{
    float4 position : SV_Position;
    nointerpolation float4 color : Color;
    nointerpolation float4 outline_color : OutlineColor;
    float2 uv : UV;
    nointerpolation float outline_thickness : OutlineThickness;
    nointerpolation bool has_texture : HasTexture;
};

static const int HAS_TEXTURE_FLAG = 1 << 0;
static const int IS_UI_FLAG = 1 << 1;
static const int IS_WORLD_FLAG = 1 << 2;
static const int IGNORE_CAMERA_ZOOM_FLAG = 1 << 3;

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

    const int flags = inp.i_flags;
    const bool is_ui = (flags & IS_UI_FLAG) == IS_UI_FLAG;
    const bool ignore_camera_zoom = (flags & IGNORE_CAMERA_ZOOM_FLAG) == IGNORE_CAMERA_ZOOM_FLAG;
    const bool is_world = (flags & IS_WORLD_FLAG) == IS_WORLD_FLAG;
    const bool has_texture = (flags & HAS_TEXTURE_FLAG) == HAS_TEXTURE_FLAG;

    const float4x4 mvp = mul(is_ui ? u_screen_projection : ignore_camera_zoom ? u_nozoom_view_projection : u_view_projection, transform);
    const float4 uv_offset_scale = inp.i_uv_offset_scale;
    const float2 position = inp.position;
    const float max_depth = is_world ? u_max_world_depth : u_max_depth;
    const float order = inp.i_position.z / max_depth;

    VSOutput outp;
    outp.position = mul(mvp, float4(position, 0.0, 1.0));
    outp.position.z = order;
    outp.uv = position * uv_offset_scale.zw + uv_offset_scale.xy;
    outp.color = inp.i_color;
    outp.outline_color = inp.i_outline_color;
    outp.outline_thickness = inp.i_outline_thickness;
    outp.has_texture = has_texture;

	return outp;
}

Texture2D Texture : register(t3);
SamplerState Sampler : register(s4);

float4 PS(VSOutput inp) : SV_Target
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

    clip(color.a - 0.5);

    return color;
};

