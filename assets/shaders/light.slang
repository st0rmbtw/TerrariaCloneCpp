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
    float3 color;
    float _padding;
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
    uint lights_count;
}

[shader("compute")]
[numthreads(32, 1, 1)]
void CSComputeLightSetLightSources(uint3 gid : SV_GroupID, uint3 tid : SV_GroupThreadID)
{
    const uint groupFlat = gid.y * 512u + gid.x;
    const uint index = groupFlat * 32u + tid.x;
    if (index >= lights_count)
        return;

    const Light light = LightBuffer[index];

    const uint2 pos = light.pos - blur_offset + texture_offset;

    const uint2 half_size = light.size / 2;

    for (uint x = 0; x < light.size.x; ++x) {
        for (uint y = 0; y < light.size.y; ++y) {
            const uint2 p = pos - half_size + uint2(x, y);
            float3 current_color = LightTexture[p].rgb;
            LightTexture[p] = float4(max(light.color, current_color), 1.0);
        }
    }
}

[shader("compute")]
[numthreads(32, 1, 1)]
void CSComputeLightVertical(uint3 gid : SV_GroupID, uint3 tid : SV_GroupThreadID) {
    const uint groupFlat = gid.y * 512u + gid.x;
    const uint x = groupFlat * 32u + tid.x;
    if (x >= blur_size.x)
        return;

    float3 prev_light = float3(0.0, 0.0, 0.0);
    float prev_decay = 0.0;

    float3 prev_light2 = float3(0.0, 0.0, 0.0);
    float prev_decay2 = 0.0;

    const uint height = blur_size.y;
    const uint max_y = blur_offset.y + blur_size.y;

    for (uint y = 0; y < blur_size.y; ++y) {
        blur(uint2(blur_offset.x + x, blur_offset.y + y), texture_offset + uint2(x, y), prev_light, prev_decay);
        blur(uint2(blur_offset.x + x, max_y - 1 - y), texture_offset + uint2(x, height - 1 - y), prev_light2, prev_decay2);
    }
}

[shader("compute")]
[numthreads(32, 1, 1)]
void CSComputeLightHorizontal(uint3 gid : SV_GroupID, uint3 tid : SV_GroupThreadID) {
    const uint groupFlat = gid.y * 512u + gid.x;
    const uint y = groupFlat * 32u + tid.x;
    if (y >= blur_size.y)
        return;

    float3 prev_light = float3(0.0, 0.0, 0.0);
    float prev_decay = 0.0;

    float3 prev_light2 = float3(0.0, 0.0, 0.0);
    float prev_decay2 = 0.0;

    const uint width = blur_size.x;
    const uint max_x = blur_offset.x + blur_size.x;

    for (uint x = 0; x < width; ++x) {
        blur(uint2(blur_offset.x + x, blur_offset.y + y), texture_offset + uint2(x, y), prev_light, prev_decay);
        blur(uint2(max_x - 1 - x, blur_offset.y + y), texture_offset + uint2(width - 1 - x, y), prev_light2, prev_decay2);
    }
}