#ifdef SLANG_COMPILING
    #ifndef DEF_SUBDIVISION
        #error "Define DEF_SUBDIVISION macro."
    #endif

    #ifndef DEF_SOLID_DECAY
        #error "Define DEF_SOLID_DECAY macro."
    #endif

    #ifndef DEF_AIR_DECAY
        #error "Define DEF_AIR_DECAY macro."
    #endif
#else
    #define DEF_SUBDIVISION 1
    #define DEF_SOLID_DECAY 1
    #define DEF_AIR_DECAY 1
#endif

struct Light {
    float4 color;
    uint2 pos;
    uint2 size;
};

[vk::binding(4)]
StructuredBuffer<Light> LightBuffer : register(t4);

[vk::binding(5)]
Texture2D<uint> TileTexture : register(t5);

[vk::binding(6)]
RWTexture2D<unorm float4> LightTexture : register(u6);

static const float LIGHT_EPSILON = 0.0185f;

static float get_decay(uint2 pos) {
    const uint tile = TileTexture[pos / DEF_SUBDIVISION].r;
    return select(tile == 1, DEF_SOLID_DECAY, DEF_AIR_DECAY);
}

static void blur(uint2 pos, uint2 texture_pos, inout float3 prev_light, inout float prev_decay) {
    float4 this_light = LightTexture[texture_pos];

    prev_light = select(prev_light < LIGHT_EPSILON, 0.0f, prev_light);
    
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
    
    LightTexture[texture_pos] = this_light;
    
    prev_light = prev_light * prev_decay;
    prev_decay = get_decay(pos);
}

[vk::binding(2)]
cbuffer UniformBuffer : register(b2) {
    uint2 texture_offset;
    uint2 blur_offset;
    uint2 blur_size;
}

[shader("compute")]
[numthreads(1, 1, 1)]
void CSComputeLightSetLightSources(uint3 thread_id : SV_DispatchThreadID)
{
    const Light light = LightBuffer[thread_id.x];
    const uint width = light.size.x;
    const uint height = light.size.y;

    const uint2 pos = light.pos - blur_offset + texture_offset;

    for (uint x = 0; x < width; ++x) {
        for (uint y = 0; y < height; ++y) {
            LightTexture[pos + uint2(x, y)] = float4(light.color.rgb, 1.0);
        }
    }
}

[shader("compute")]
[numthreads(32, 1, 1)]
void CSComputeLightVertical(
    uint3 thread_id: SV_DispatchThreadID
) {
    const uint x = blur_offset.x + thread_id.x;

    float3 prev_light = float3(0.0, 0.0, 0.0);
    float prev_decay = 0.0;

    float3 prev_light2 = float3(0.0, 0.0, 0.0);
    float prev_decay2 = 0.0;

    const uint height = blur_size.y;
    const uint max_y = blur_offset.y + blur_size.y;

    for (uint y = 0; y < blur_size.y; ++y) {
        blur(uint2(x, blur_offset.y + y), texture_offset + uint2(thread_id.x, y), prev_light, prev_decay);
        blur(uint2(x, max_y - 1 - y), texture_offset + uint2(thread_id.x, height - 1 - y), prev_light2, prev_decay2);
    }
}

[shader("compute")]
[numthreads(32, 1, 1)]
void CSComputeLightHorizontal(
    uint3 thread_id: SV_DispatchThreadID,
) {
    const uint y = blur_offset.y + thread_id.x;

    float3 prev_light = float3(0.0, 0.0, 0.0);
    float prev_decay = 0.0;

    float3 prev_light2 = float3(0.0, 0.0, 0.0);
    float prev_decay2 = 0.0;

    const uint width = blur_size.x;
    const uint max_x = blur_offset.x + blur_size.x;

    for (uint x = 0; x < width; ++x) {
        blur(uint2(blur_offset.x + x, y), texture_offset + uint2(x, thread_id.x), prev_light, prev_decay);
        blur(uint2(max_x - 1 - x, y), texture_offset + uint2(width - 1 - x, thread_id.x), prev_light2, prev_decay2);
    }
}