#include <metal_stdlib>

using namespace metal;

struct Constants
{
    float4x4 screen_projection;
    float4x4 view_projection;
    float4x4 nonscale_view_projection;
    float4x4 nonscale_projection;
    float4x4 transform_matrix;
    float4x4 inv_view_proj;
    float2 camera_position;
    float2 window_size;
    float max_depth;
    float max_world_depth;
};

struct VertexIn
{
    float2 position     [[attribute(0)]];
    float2 texture_size [[attribute(1)]];

    float2 i_position   [[attribute(2)]];
    float2 i_size       [[attribute(3)]];
    float2 i_tex_size   [[attribute(4)]];
    float2 i_speed      [[attribute(5)]];
    uint   i_id         [[attribute(6)]];
    int    i_flags      [[attribute(7)]];
};

struct VertexOut
{
    float4 position     [[position]];
    float2 uv;
    float2 texture_size [[flat]];
    float2 size         [[flat]];
    float2 tex_size     [[flat]];
    float2 speed        [[flat]];
    float2 offset       [[flat]];
    uint   id           [[flat]];
    int    nonscale     [[flat]];
};

float fmodb(float a, float b) {
    float r = fmod(a, b);
    return r < 0.0 ? r + b : r;
}

constant constexpr int IGNORE_CAMERA_ZOOM_FLAG = 1 << 0;

vertex VertexOut VS(
    VertexIn inp [[stage_in]],
    constant Constants& constants [[buffer(2)]]
) {
    const int flags = inp.i_flags;
    const bool ignore_camera_zoom = (flags & IGNORE_CAMERA_ZOOM_FLAG) == IGNORE_CAMERA_ZOOM_FLAG;

    const float4x4 view_proj = ignore_camera_zoom ? constants.nonscale_view_projection : constants.view_projection;
    const float4x4 proj_model = constants.nonscale_projection * constants.transform_matrix;
    const float2 offset = (proj_model * float4(constants.camera_position.x, constants.camera_position.y, 0.0, 1.0)).xy;

    const float2 position = inp.i_position.xy + inp.i_size * inp.position;

    VertexOut outp;
    outp.position = view_proj * float4(position.x, position.y, 0.0, 1.0);
    outp.position.z = 0.0;
    outp.uv = inp.position;
    outp.texture_size = inp.texture_size;
    outp.tex_size = inp.i_tex_size;
    outp.size = inp.i_size;
    outp.speed = inp.i_speed;
    outp.offset = offset;
    outp.id = inp.i_id;
    outp.nonscale = flags & IGNORE_CAMERA_ZOOM_FLAG;

    return outp;
}

float2 scroll(
    float2 speed,
    float2 uv,
    float2 offset,
    float2 texture_size,
    float2 tex_size,
    float2 size,
    bool nonscale
) {
    float2 new_offset = float2(-offset.x, offset.y);

    if (!nonscale) {
        speed.x = speed.x / (size.x / tex_size.x);
    }

    float2 new_uv = uv - (new_offset * float2(speed.x, 0.0));
    if (!nonscale) {
        new_uv = new_uv * (size / tex_size);
    }

    new_uv.x = fmodb(new_uv.x, 1.0);
    new_uv.y = fmodb(new_uv.y, 1.0);
    
    if (id == 0) {
        float x = new_uv.x;
        new_uv.x = new_uv.y;
        new_uv.y = x;
    }

    new_uv = new_uv * tex_size / texture_size;

    return new_uv;
}

fragment float4 PS(
    VertexOut inp [[stage_in]],
    constant Constants& constants [[buffer(2)]],
    texture2d_array<float> texture [[texture(3)]],
    sampler texture_sampler [[sampler(4)]]
) {
    float2 uv = scroll(inp.speed, inp.uv, inp.offset, inp.texture_size, inp.tex_size, inp.size, inp.nonscale > 0);
    float4 color = texture.sample(texture_sampler, uv, inp.id);

    if (color.a < 0.05) discard_fragment();

    return color;
}