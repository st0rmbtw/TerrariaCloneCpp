#include <metal_stdlib>

using namespace metal;

struct Constants
{
    float4x4 screen_projection;
    float4x4 view_projection;
    float4x4 nonscale_view_projection;
    float4x4 nonscale_projection;
    float4x4 transform_matrix;
    float2 camera_position;
    float2 window_size;
    float max_depth;
    float max_world_depth;
};

struct VertexIn
{
    float2 position [[attribute(0)]];
    float2 uv       [[attribute(1)]];
    float2 size     [[attribute(2)]];
    float2 tex_size [[attribute(3)]];
    float2 speed    [[attribute(4)]];
    int    nonscale [[attribute(5)]];
};

struct VertexOut
{
    float4 position [[position]];
    float2 uv;
    float2 size     [[flat]];
    float2 tex_size [[flat]];
    float2 speed    [[flat]];
    float2 offset   [[flat]];
    int    nonscale [[flat]];
};

float fmodb(float a, float b) {
    float r = fmod(a, b);
    return r < 0.0 ? r + b : r;
}

vertex VertexOut VS(
    VertexIn inp [[stage_in]],
    constant Constants& constants [[buffer(1)]]
) {
    float4x4 mvp = inp.nonscale > 0 ? constants.nonscale_view_projection : constants.view_projection;
    float4x4 proj_model = constants.nonscale_projection * constants.transform_matrix;
    float2 offset = (proj_model * float4(constants.camera_position.x, constants.camera_position.y, 0.0, 1.0)).xy;

    VertexOut outp;
    outp.position = view_proj * float4(inp.position.x, inp.position.y, 0.0, 1.0);
    outp.position.z = 0.0;
    outp.uv = inp.uv;
    outp.tex_size = inp.tex_size;
    outp.size = inp.size;
    outp.speed = inp.speed;
    outp.offset = offset;
    outp.nonscale = inp.nonscale;

    return outp;
}

float2 scroll(
    float2 speed,
    float2 uv,
    float2 offset,
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

    return new_uv;
}

fragment float4 PS(
    VertexOut inp [[stage_in]],
    constant Constants& constants [[buffer(1)]],
    texture2d<float> texture [[texture(2)]],
    sampler texture_sampler [[sampler(3)]]
) {
    float2 uv = scroll(inp.speed, inp.uv, inp.offset, inp.tex_size, inp.size, inp.nonscale > 0);
    float4 color = texture.sample(texture_sampler, uv);

    if (color.a < 0.05) discard_fragment();

    return color;
}