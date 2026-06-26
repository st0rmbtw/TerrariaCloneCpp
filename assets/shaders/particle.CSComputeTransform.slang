#include "common.slang"

[vk::binding(2)]
cbuffer GlobalUniformBuffer : register( b2 )
{
    GlobalUniforms uniforms;
};

static const float2 PARTICLE_SIZE = float2(8.0, 8.0);

[vk::binding(5)]
RWStructuredBuffer<float4> TransformBuffer : register(u5);

[vk::binding(6)]
StructuredBuffer<float2> PositionBuffer : register(t6);

[vk::binding(7)]
StructuredBuffer<float4> RotationBuffer : register(t7);

[vk::binding(8)]
StructuredBuffer<float> ScaleBuffer : register(t8);

[shader("compute")]
[numthreads(32, 1, 1)]
void CSComputeTransform(uint3 tid : SV_DispatchThreadID)
{
    const uint id = tid.x;

    const float2 position = PositionBuffer[id];
    const float4 rotation = RotationBuffer[id];
    const float scale = ScaleBuffer[id];

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

    const float4x4 mvp = mul(uniforms.view_projection, transform);

    TransformBuffer[id * 4 + 0] = mvp[0];
    TransformBuffer[id * 4 + 1] = mvp[1];
    TransformBuffer[id * 4 + 2] = mvp[2];
    TransformBuffer[id * 4 + 3] = mvp[3];
}

