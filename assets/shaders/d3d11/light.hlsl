cbuffer GlobalUniformBuffer : register( b3 )
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

cbuffer Uniforms : register(b2) {
    uint2 uniform_min;
    uint2 uniform_max;
}

struct Light {
    float3 color;
    uint2 pos;
    uint2 size;
};

StructuredBuffer<Light> LightBuffer : register(t4);
Texture2D<uint> TileTexture : register(t5);
RWTexture2D<unorm float4> LightTexture : register(u6);

#if DEF_SUBDIVISION == 8
    static const float DECAY_THROUGH_SOLID = 0.92;
    static const float DECAY_THROUGH_AIR = 0.975;
#elif DEF_SUBDIVISION == 4
    static const float DECAY_THROUGH_SOLID = 0.84;
    static const float DECAY_THROUGH_AIR = 0.942;
#else
    static const float DECAY_THROUGH_SOLID = 0.56;
    static const float DECAY_THROUGH_AIR = 0.91;
#endif

static const float LIGHT_EPSILON = 0.0185f;

float get_decay(uint2 pos) {
    uint tile = TileTexture[pos / DEF_SUBDIVISION].r;
    if (tile == 1u) {
        return DECAY_THROUGH_SOLID;
    } else {
        return DECAY_THROUGH_AIR;
    }
}

void blur(uint2 pos, inout float3 prev_light, inout float prev_decay) {
    float4 this_light = LightTexture[pos];

    prev_light = prev_light < LIGHT_EPSILON ? 0.0f : prev_light;
    
    if (prev_light.x < this_light.x) {
        prev_light.x = this_light.x;
    } else {
        this_light.x = prev_light.x;
    }
    
    if (prev_light.y < this_light.y) {
        prev_light.y = this_light.y;
    } else {
        this_light.y = prev_light.y;
    }
    
    if (prev_light.z < this_light.z) {
        prev_light.z = this_light.z;
    } else {
        this_light.z = prev_light.z;
    }
    
    LightTexture[pos] = this_light;
    
    prev_light = prev_light * prev_decay;
    prev_decay = get_decay(pos);
}

[numthreads(1, 1, 1)]
void CSComputeLightSetLightSources(uint3 thread_id : SV_DispatchThreadID)
{
    const Light light = LightBuffer[thread_id.x];
    const uint width = light.size.x;
    const uint height = light.size.y;

    for (uint x = 0; x < width; ++x) {
        for (uint y = 0; y < height; ++y) {
            LightTexture[light.pos + uint2(x, y)] = float4(light.color, 1.0);
        }
    }
}

[numthreads(32, 1, 1)]
void CSComputeLightVertical(uint3 thread_id : SV_DispatchThreadID)
{
    const uint x = uniform_min.x + thread_id.x;

    float3 prev_light = float3(0.0, 0.0, 0.0);
    float prev_decay = 0.0;

    float3 prev_light2 = float3(0.0, 0.0, 0.0);
    float prev_decay2 = 0.0;

    const uint height = uniform_max.y - uniform_min.y;

    for (uint y = 0; y < height; ++y) {
        blur(uint2(x, uniform_min.y + y), prev_light, prev_decay);
        blur(uint2(x, uniform_max.y - 1 - y), prev_light2, prev_decay2);
    }
}

[numthreads(32, 1, 1)]
void CSComputeLightHorizontal(uint3 thread_id : SV_DispatchThreadID)
{
    const uint y = uniform_min.y + thread_id.x;

    float3 prev_light = float3(0.0, 0.0, 0.0);
    float prev_decay = 0.0;

    float3 prev_light2 = float3(0.0, 0.0, 0.0);
    float prev_decay2 = 0.0;

    const uint width = uniform_max.x - uniform_min.x;

    for (uint x = 0; x < width; ++x) {
        blur(uint2(uniform_min.x + x, y), prev_light, prev_decay);
        blur(uint2(uniform_max.x - 1 - x, y), prev_light2, prev_decay2);
    }
}