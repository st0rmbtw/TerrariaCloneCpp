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
};

struct VertexIn
{
    float2 position     [[attribute(0)]];
    float2 inv_tex_size [[attribute(1)]];
    float2 tex_size     [[attribute(2)]];
    float2 i_uv         [[attribute(3)]];
    float  i_depth      [[attribute(4)]];
    uint   i_id         [[attribute(5)]];
    uint   i_is_world   [[attribute(6)]];
};

struct VertexOut
{
    float4 position [[position]];
    float2 uv;
};

vertex VertexOut VS(
    VertexIn inp                       [[stage_in]],
    constant Constants& constants      [[buffer(2)]],
    const device float4* transforms    [[buffer(5)]],
    uint id                          [[instance_id]]
) {
    float2 position = inp.position;
    float4x4 mvp = float4x4(
        transforms[id * 4 + 0],
        transforms[id * 4 + 1],
        transforms[id * 4 + 2],
        transforms[id * 4 + 3]
    );

    float2 uv = inp.i_uv / inp.tex_size;

    const bool is_world = inp.i_is_world > 0;

    VertexOut outp;
    outp.position = mvp * float4(position, 0.0, 1.0);
    outp.position.z = inp.i_depth;
    outp.uv = uv + position * inp.inv_tex_size;

    return outp;
}

constant constexpr float2 PARTICLE_SIZE = float2(8.0, 8.0);

kernel void CSComputeTransform(
    constant Constants&  constants  [[buffer(2)]],
    device float4*       transforms [[buffer(5)]],
    const device float2* positions  [[buffer(6)]],
    const device float4* rotations  [[buffer(7)]],
    const device float*  scales     [[buffer(8)]],
    uint3                thread_id  [[thread_position_in_grid]]
) {
    const uint id = thread_id.y * 512 + thread_id.x;

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
        float4(1.0 - 2.0 * (qyy + qzz), 2.0 * (qxy + qwz), 2.0 * (qxz - qwy), 0.0),
        float4(2.0 * (qxy - qwz), 1.0 - 2.0 * (qxx +  qzz), 2.0 * (qyz + qwx), 0.0),
        float4(2.0 * (qxz + qwy), 2.0 * (qyz - qwx), 1.0 - 2.0 * (qxx +  qyy), 0.0),
        float4(0.0, 0.0, 0.0, 1.0)
    );

    const float2 size = PARTICLE_SIZE * scale;
    const float2 offset = -0.5 * size;

    float4x4 transform = float4x4(
        float4(1.0,        0.0,        0.0, 0.0),
        float4(0.0,        1.0,        0.0, 0.0),
        float4(0.0,        0.0,        1.0, 0.0),
        float4(position.x, position.y, 0.0, 1.0)
    );

    transform = transform * rotation_matrix;

    // translate
    transform[3] = transform[0] * offset[0] + transform[1] * offset[1] + transform[2] * 0.0 + transform[3];

    // scale
    transform[0] = transform[0] * size[0];
    transform[1] = transform[1] * size[1];

    const float4x4 mvp = constants.view_projection * transform;

    transforms[id * 4 + 0] = mvp[0];
    transforms[id * 4 + 1] = mvp[1];
    transforms[id * 4 + 2] = mvp[2];
    transforms[id * 4 + 3] = mvp[3];
}

fragment float4 PS(
    VertexOut inp [[stage_in]],
    texture2d<float> texture [[texture(3)]],
    sampler texture_sampler [[sampler(4)]]
) {
    float4 color = texture.sample(texture_sampler, inp.uv);

    return color;
}