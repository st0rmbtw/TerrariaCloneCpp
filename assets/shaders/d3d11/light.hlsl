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
    static const float DECAY_THROUGH_SOLID = 0.86;
    static const float DECAY_THROUGH_AIR = 0.975;
#elif DEF_SUBDIVISION == 4
    static const float DECAY_THROUGH_SOLID = 0.78;
    static const float DECAY_THROUGH_AIR = 0.91;
#else
    static const float DECAY_THROUGH_SOLID = 0.56;
    static const float DECAY_THROUGH_AIR = 0.91;
#endif

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
    
    bool this_light_changed = false;
    
    if (prev_light.x < this_light.x) {
        prev_light.x = this_light.x;
    } else {
        this_light.x = prev_light.x;
        this_light_changed = true;
    }
    
    if (prev_light.y < this_light.y) {
        prev_light.y = this_light.y;
    } else {
        this_light.y = prev_light.y;
        this_light_changed = true;
    }
    
    if (prev_light.z < this_light.z) {
        prev_light.z = this_light.z;
    } else {
        this_light.z = prev_light.z;
        this_light_changed = true;
    }
    
    if (this_light_changed) {
        LightTexture[pos] = this_light;
    }
    
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

[numthreads(16, 1, 1)]
void CSComputeLightTopToBottom(uint3 thread_id : SV_DispatchThreadID)
{
    const uint x = uniform_min.x + thread_id.x;

    float3 prev_light = float3(0.0, 0.0, 0.0);
    float prev_decay = 0.0;

    for (uint y = uniform_min.y; y < uniform_max.y; ++y) {
        blur(uint2(x, y), prev_light, prev_decay);
    }
}

[numthreads(16, 1, 1)]
void CSComputeLightBottomToTop(uint3 thread_id : SV_DispatchThreadID)
{
    const uint x = uniform_min.x + thread_id.x;

    float3 prev_light = float3(0.0, 0.0, 0.0);
    float prev_decay = 0.0;

    for (uint y = uniform_max.y - 1; y >= uniform_min.y; --y) {
        blur(uint2(x, y), prev_light, prev_decay);
    }
}

[numthreads(16, 1, 1)]
void CSComputeLightLeftToRight(uint3 thread_id : SV_DispatchThreadID)
{
    const uint y = uniform_min.y + thread_id.x;

    float3 prev_light = float3(0.0, 0.0, 0.0);
    float prev_decay = 0.0;

    for (uint x = uniform_min.x; x < uniform_max.x; ++x) {
        blur(uint2(x, y), prev_light, prev_decay);
    }
}

[numthreads(16, 1, 1)]
void CSComputeLightRightToLeft(uint3 thread_id : SV_DispatchThreadID)
{
    const uint y = uniform_min.y + thread_id.x;

    float3 prev_light = float3(0.0, 0.0, 0.0);
    float prev_decay = 0.0;

    for (uint x = uniform_max.x - 1; x >= uniform_min.x; --x) {
        blur(uint2(x, y), prev_light, prev_decay);
    }
}
