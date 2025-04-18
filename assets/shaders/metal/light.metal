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

struct Uniforms {
    uint2 uniform_min;
    uint2 uniform_max;
};

struct Light {
    float4 color;
    uint2 pos;
    uint2 size;
};

constant constexpr float LIGHT_EPSILON = 0.0185;

static float get_decay(uint tile, uint2 pos) {
    return tile == 1u ? DEF_SOLID_DECAY : DEF_AIR_DECAY;
}

static float4 blur(uint2 pos, thread float3* prev_light, float prev_decay, float4 this_light) {
    prev_light->x = prev_light->x < LIGHT_EPSILON ? 0.0 : prev_light->x;
    prev_light->y = prev_light->y < LIGHT_EPSILON ? 0.0 : prev_light->y;
    prev_light->z = prev_light->z < LIGHT_EPSILON ? 0.0 : prev_light->z;
    
    if (prev_light->x < this_light.x) {
        prev_light->x = this_light.x;
    } else {
        this_light.x = prev_light->x;
    }
    
    if (prev_light->y < this_light.y) {
        prev_light->y = this_light.y;
    } else {
        this_light.y = prev_light->y;
    }
    
    if (prev_light->z < this_light.z) {
        prev_light->z = this_light.z;
    } else {
        this_light.z = prev_light->z;
    }
    
    *prev_light = *prev_light * prev_decay;

    return this_light;
}

kernel void CSComputeLightSetLightSources(
    constant Constants& constants                  [[buffer(3)]],
    constant Light* light_buffer               [[buffer(4)]],
    texture2d<uint> tile_texture                   [[texture(5)]],
    texture2d<float, access::write> light_texture [[texture(6)]],
    uint3 thread_id [[thread_position_in_grid]]
) {
    const Light light = light_buffer[thread_id.x];
    const uint width = light.size.x;
    const uint height = light.size.y;

    for (uint x = 0; x < width; ++x) {
        for (uint y = 0; y < height; ++y) {
            light_texture.write(float4(light.color.rgb, 1.0), light.pos + uint2(x, y));
        }
    }
}

kernel void CSComputeLightHorizontal(
    constant Uniforms& uniforms                    [[buffer(2)]],
    constant Constants& constants                  [[buffer(3)]],
    texture2d<uint> tile_texture                   [[texture(5)]],
    texture2d<float, access::read_write> light_texture [[texture(6)]],
    uint3 thread_id [[thread_position_in_grid]]
) {
    const uint id = thread_id.y * 512 + thread_id.x;
    const uint x = uniforms.uniform_min.x + id;

    float3 prev_light = float3(0.0, 0.0, 0.0);
    float prev_decay = 0.0;

    float3 prev_light2 = float3(0.0, 0.0, 0.0);
    float prev_decay2 = 0.0;

    const uint height = uniforms.uniform_max.y - uniforms.uniform_min.y;

    uint2 pos;
    uint tile;
    for (uint y = 0; y < height; ++y) {
        pos = uint2(x, uniforms.uniform_min.y + y);
        light_texture.write(blur(pos, &prev_light, prev_decay, light_texture.read(pos)), pos);
        tile = tile_texture.read(pos / DEF_SUBDIVISION).r;
        prev_decay = get_decay(tile, pos);

        pos = uint2(x, uniforms.uniform_max.y - 1 - y);
        light_texture.write(blur(pos, &prev_light2, prev_decay2, light_texture.read(pos)), pos);
        tile = tile_texture.read(pos / DEF_SUBDIVISION).r;
        prev_decay2 = get_decay(tile, pos);
    }
}

kernel void CSComputeLightVertical(
    constant Uniforms& uniforms                    [[buffer(2)]],
    constant Constants& constants                  [[buffer(3)]],
    texture2d<uint> tile_texture                   [[texture(5)]],
    texture2d<float, access::read_write> light_texture [[texture(6)]],
    uint3 thread_id [[thread_position_in_grid]]
) {
    const uint id = thread_id.y * 512 + thread_id.x;
    const uint y = uniforms.uniform_min.y + id;

    float3 prev_light = float3(0.0, 0.0, 0.0);
    float prev_decay = 0.0;

    float3 prev_light2 = float3(0.0, 0.0, 0.0);
    float prev_decay2 = 0.0;

    const uint width = uniforms.uniform_max.x - uniforms.uniform_min.x;

    uint2 pos;
    uint tile;
    for (uint x = 0; x < width; ++x) {
        pos = uint2(uniforms.uniform_min.x + x, y);
        light_texture.write(blur(pos, &prev_light, prev_decay, light_texture.read(pos)), pos);
        tile = tile_texture.read(pos / DEF_SUBDIVISION).r;
        prev_decay = get_decay(tile, pos);

        pos = uint2(uniforms.uniform_max.x - 1 - x, y);
        light_texture.write(blur(pos, &prev_light2, prev_decay2, light_texture.read(pos)), pos);
        tile = tile_texture.read(pos / DEF_SUBDIVISION).r;
        prev_decay2 = get_decay(tile, pos);
    }
}