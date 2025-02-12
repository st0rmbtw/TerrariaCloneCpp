#include "particles.hpp"

#include <immintrin.h>
#include <xmmintrin.h>

#include <tracy/Tracy.hpp>

#include "renderer/renderer.hpp"
#include "engine/defines.hpp"
#include "engine/time/time.hpp"

#include "constants.hpp"
#include "utils.hpp"

#ifdef PLATFORM_WINDOWS
    #include <corecrt_malloc.h>
    #define ALIGNED_ALLOC(size, alignment) _aligned_malloc((size), (alignment))
    #define ALIGNED_FREE(ptr) _aligned_free((ptr))
#else
    #include <stdlib.h>
    #define ALIGNED_ALLOC(size, alignment) aligned_alloc((alignment), (size))
    #define ALIGNED_FREE(ptr) free((ptr))
#endif

using Constants::MAX_PARTICLES_COUNT;

namespace ParticleFlags {
    constexpr uint8_t Active = 1 << 0;
    constexpr uint8_t Gravity = 1 << 1;
    constexpr uint8_t EmitsLight = 1 << 2;
    constexpr uint8_t InWorldLayer = 1 << 3;
}

static struct ParticlesState {
    float* position;
    float* velocity;
    float* lifetime;
    float* max_lifetime;
    float* custom_scale;
    float* scale;
    float* rotation;
    float* rotation_speed;
    float* initial_light_color;
    float* light_color;
    uint8_t* flags;
    Particle::Type* type;
    uint8_t* variant;

    size_t active_count = 0;
} state;

template <typename T>
static T* checked_aligned_alloc(size_t count, size_t alignment) {
    T* _ptr = (T*) ALIGNED_ALLOC(count * sizeof(T), alignment);
    if (_ptr == nullptr && count > 0) {
        LOG_ERROR("Out of memory");
        abort();
    }
    return _ptr;
}

void ParticleManager::Init() {
    ZoneScopedN("ParticleManager::Init");

#if defined(__AVX__)
    state.position       = checked_aligned_alloc<float>(MAX_PARTICLES_COUNT * 2, sizeof(__m256));
    state.velocity       = checked_aligned_alloc<float>(MAX_PARTICLES_COUNT * 2, sizeof(__m256));
    state.lifetime       = checked_aligned_alloc<float>(MAX_PARTICLES_COUNT,     sizeof(__m256));
    state.rotation       = checked_aligned_alloc<float>(MAX_PARTICLES_COUNT * 4, sizeof(__m256));
    state.rotation_speed = checked_aligned_alloc<float>(MAX_PARTICLES_COUNT * 4, sizeof(__m256));
#else
    state.position       = checked_alloc<float>(MAX_PARTICLES_COUNT * 2);
    state.velocity       = checked_alloc<float>(MAX_PARTICLES_COUNT * 2);
    state.lifetime       = checked_alloc<float>(MAX_PARTICLES_COUNT);
    state.rotation       = checked_alloc<float>(MAX_PARTICLES_COUNT * 4);
    state.rotation_speed = checked_alloc<float>(MAX_PARTICLES_COUNT * 4);
#endif
    state.max_lifetime        = checked_alloc<float>(MAX_PARTICLES_COUNT);
    state.custom_scale        = checked_alloc<float>(MAX_PARTICLES_COUNT);
    state.scale               = checked_alloc<float>(MAX_PARTICLES_COUNT);
    state.initial_light_color = checked_alloc<float>(MAX_PARTICLES_COUNT * 3);
    state.light_color         = checked_alloc<float>(MAX_PARTICLES_COUNT * 3);
    state.flags               = checked_alloc<uint8_t>(MAX_PARTICLES_COUNT);
    state.type                = checked_alloc<Particle::Type>(MAX_PARTICLES_COUNT);
    state.variant             = checked_alloc<uint8_t>(MAX_PARTICLES_COUNT);

    memset(state.flags, 0, MAX_PARTICLES_COUNT * sizeof(uint8_t));
}

void ParticleManager::SpawnParticle(const ParticleBuilder& builder) {
    ZoneScopedN("ParticleManager::SpawnParticle");

    const uint32_t index = state.active_count;
    ParticleData particle_data = builder.build();

    state.position[index * 2 + 0] = particle_data.position.x;
    state.position[index * 2 + 1] = particle_data.position.y;

    state.velocity[index * 2 + 0] = particle_data.velocity.x;
    state.velocity[index * 2 + 1] = particle_data.velocity.y;

    state.lifetime[index] = particle_data.lifetime;
    state.max_lifetime[index] = particle_data.lifetime;
    state.custom_scale[index] = particle_data.custom_scale;
    state.scale[index] = particle_data.scale;

    state.rotation_speed[index * 4 + 0] = particle_data.rotation_speed.x;
    state.rotation_speed[index * 4 + 1] = particle_data.rotation_speed.y;
    state.rotation_speed[index * 4 + 2] = particle_data.rotation_speed.z;
    state.rotation_speed[index * 4 + 3] = particle_data.rotation_speed.w;

    state.rotation[index * 4 + 0] = particle_data.rotation.x;
    state.rotation[index * 4 + 1] = particle_data.rotation.y;
    state.rotation[index * 4 + 2] = particle_data.rotation.z;
    state.rotation[index * 4 + 3] = particle_data.rotation.w;

    const bool emits_light = particle_data.light_color.has_value();

    uint8_t flags = ParticleFlags::Active;
    flags |= ParticleFlags::Gravity * particle_data.gravity;
    flags |= ParticleFlags::InWorldLayer * particle_data.is_world;
    flags |= ParticleFlags::EmitsLight * emits_light;

    state.flags[index] = flags;

    state.light_color[index * 3 + 0] = emits_light ? particle_data.light_color->x : 0.0f;
    state.light_color[index * 3 + 1] = emits_light ? particle_data.light_color->y : 0.0f;
    state.light_color[index * 3 + 2] = emits_light ? particle_data.light_color->z : 0.0f;

    state.initial_light_color[index * 3 + 0] = emits_light ? particle_data.light_color->x : 0.0f;
    state.initial_light_color[index * 3 + 1] = emits_light ? particle_data.light_color->y : 0.0f;
    state.initial_light_color[index * 3 + 2] = emits_light ? particle_data.light_color->z : 0.0f;

    state.type[index] = particle_data.type;
    state.variant[index] = particle_data.variant;

    state.active_count = ++state.active_count % MAX_PARTICLES_COUNT;
}

void ParticleManager::Draw() {
    ZoneScopedN("ParticleManager::Draw");

    const uint32_t main_depth = GameRenderer::GetMainOrderIndex();
    const uint32_t world_depth = GameRenderer::GetWorldOrderIndex();

    for (size_t i = 0; i < state.active_count; ++i) {
        const glm::vec2 position(state.position[i * 2 + 0], state.position[i * 2 + 1]);

        const glm::quat& rotation = *reinterpret_cast<const glm::quat*>(&state.rotation[i * 4]);

        const float scale = state.scale[i]; 
        const Particle::Type type = state.type[i];
        const uint8_t variant = state.variant[i];
        const bool world = BITFLAG_CHECK(state.flags[i], ParticleFlags::InWorldLayer);

        GameRenderer::DrawParticle(position, rotation, scale, type, variant, world ? world_depth : main_depth, world);
    }
}

void ParticleManager::Update(World& world) {
    ZoneScopedN("ParticleManager::Update");

    for (size_t i = 0; i < state.active_count; ++i) {
        const bool gravity = BITFLAG_CHECK(state.flags[i], ParticleFlags::Gravity);
        float& velocity_y = state.velocity[i * 2 + 1];

        if (gravity) {
            velocity_y += 0.1f;
        }
    }

    const float dt = Time::fixed_delta_seconds();
    
#if defined(__AVX__)
    // The size of a float type is 32 bit, so it can be packed as 8 into 256 bit vector.
    for (size_t i = 0; i < state.active_count; i += 8) {
        const __m256 lifetime = _mm256_load_ps(&state.lifetime[i]);
        const __m256 delta = _mm256_set1_ps(dt);
        const __m256 new_lifetime = _mm256_sub_ps(lifetime, delta);
        _mm256_store_ps(&state.lifetime[i], new_lifetime);
    }

    // The size of a vec2 type is 64 bit, so it can be packed as 4 into 256 bit vector.
    for (size_t i = 0; i < state.active_count * 2; i += 8) {
        const __m256 positionVector = _mm256_load_ps(&state.position[i]);
        const __m256 velocityVector = _mm256_load_ps(&state.velocity[i]);
        const __m256 newPosition = _mm256_add_ps(positionVector, velocityVector);
        _mm256_store_ps(&state.position[i], newPosition);
    }

    // The size of a quat type is 128 bit, so it can be packed as 2 into 256 bit vector.
    for (size_t i = 0; i < state.active_count * 4; i += 8) {
        const __m256 xyzw = _mm256_load_ps(&state.rotation[i]);
        const __m256 abcd = _mm256_load_ps(&state.rotation_speed[i]);

        __m256 wzyx = _mm256_shuffle_ps(xyzw, xyzw, _MM_SHUFFLE(0,1,2,3));
        __m256 baba = _mm256_shuffle_ps(abcd, abcd, _MM_SHUFFLE(0,1,0,1));
        __m256 dcdc = _mm256_shuffle_ps(abcd, abcd, _MM_SHUFFLE(2,3,2,3));

        /* variable names below are for parts of componens of result (X,Y,Z,W) */
        /* nX stands for -X and similarly for the other components             */

        /* znxwy  = (xb - ya, zb - wa, wd - zc, yd - xc) */
        __m256 ZnXWY = _mm256_hsub_ps(_mm256_mul_ps(xyzw, baba), _mm256_mul_ps(wzyx, dcdc));

        /* xzynw  = (xd + yc, zd + wc, wb + za, yb + xa) */
        __m256 XZYnW = _mm256_hadd_ps(_mm256_mul_ps(xyzw, dcdc), _mm256_mul_ps(wzyx, baba));

        /* _mm_shuffle_ps(XZYnW, ZnXWY, _MM_SHUFFLE(3,2,1,0)) */
        /*      = (xd + yc, zd + wc, wd - zc, yd - xc)        */
        /* _mm_shuffle_ps(ZnXWY, XZYnW, _MM_SHUFFLE(2,3,0,1)) */
        /*      = (zb - wa, xb - ya, yb + xa, wb + za)        */

        /* _mm_addsub_ps adds elements 1 and 3 and subtracts elements 0 and 2, so we get: */
        /* _mm_addsub_ps(*, *) = (xd+yc-zb+wa, xb-ya+zd+wc, wd-zc+yb+xa, yd-xc+wb+za)     */

        __m256 XZWY = _mm256_addsub_ps(_mm256_shuffle_ps(XZYnW, ZnXWY, _MM_SHUFFLE(3,2,1,0)),
                                    _mm256_shuffle_ps(ZnXWY, XZYnW, _MM_SHUFFLE(2,3,0,1)));

        /* now we only need to shuffle the components in place and return the result      */
		_mm256_store_ps(&state.rotation[i], _mm256_shuffle_ps(XZWY, XZWY, _MM_SHUFFLE(2,1,3,0)));
    }
#else
    for (size_t i = 0; i < state.active_count; i += 1) {
        state.lifetime[i] -= dt;
    }

    for (size_t i = 0; i < state.active_count * 2; i += 2) {
        state.position[i + 0] += state.velocity[i + 0];
        state.position[i + 1] += state.velocity[i + 1];
    }

    for (size_t i = 0; i < state.active_count * 4; i += 4) {
        glm::quat& rotation = *reinterpret_cast<glm::quat*>(&state.rotation[i]);
        const glm::quat& rotation_speed = *reinterpret_cast<const glm::quat*>(&state.rotation_speed[i]);
        rotation *= rotation_speed;
    }
#endif

    for (size_t i = 0; i < state.active_count; ++i) {
        const bool emits_light = BITFLAG_CHECK(state.flags[i], ParticleFlags::EmitsLight);
        if (!emits_light) continue;

        const glm::vec3 light_color = glm::vec3(
            state.light_color[i * 3 + 0],
            state.light_color[i * 3 + 1],
            state.light_color[i * 3 + 2]
        );

        const glm::vec2 position = glm::vec2(state.position[i * 2 + 0], state.position[i * 2 + 1]);

        world.add_light(Light {
            .color = light_color,
            .pos = get_lightmap_pos(position),
            .size = glm::uvec2(Constants::SUBDIVISION / 2)
        });
    }
}

void ParticleManager::DeleteExpired() {
    ZoneScopedN("ParticleManager::DeleteExpired");

    size_t i = 0;

    while (state.active_count > 0) {
        const float lifetime = state.lifetime[i];
        bool active = BITFLAG_CHECK(state.flags[i], ParticleFlags::Active);
        if (!active) break;

        if (lifetime <= 0.0f) {
            state.flags[i] = BITFLAG_REMOVE(state.flags[i], ParticleFlags::Active);
            state.active_count--;
            std::swap(state.position[i * 2], state.position[state.active_count * 2]);
            std::swap(state.position[i * 2 + 1], state.position[state.active_count * 2 + 1]);

            std::swap(state.velocity[i * 2], state.velocity[state.active_count * 2]);
            std::swap(state.velocity[i * 2 + 1], state.velocity[state.active_count * 2 + 1]);

            std::swap(state.lifetime[i], state.lifetime[state.active_count]);
            std::swap(state.max_lifetime[i], state.max_lifetime[state.active_count]);
            std::swap(state.custom_scale[i], state.custom_scale[state.active_count]);
            std::swap(state.scale[i], state.scale[state.active_count]);

            std::swap(state.rotation[i * 4 + 0], state.rotation[state.active_count * 4 + 0]);
            std::swap(state.rotation[i * 4 + 1], state.rotation[state.active_count * 4 + 1]);
            std::swap(state.rotation[i * 4 + 2], state.rotation[state.active_count * 4 + 2]);
            std::swap(state.rotation[i * 4 + 3], state.rotation[state.active_count * 4 + 3]);

            std::swap(state.rotation_speed[i * 4 + 0], state.rotation_speed[state.active_count * 4 + 0]);
            std::swap(state.rotation_speed[i * 4 + 1], state.rotation_speed[state.active_count * 4 + 1]);
            std::swap(state.rotation_speed[i * 4 + 2], state.rotation_speed[state.active_count * 4 + 2]);
            std::swap(state.rotation_speed[i * 4 + 3], state.rotation_speed[state.active_count * 4 + 3]);

            std::swap(state.light_color[i * 3 + 0], state.light_color[state.active_count * 3 + 0]);
            std::swap(state.light_color[i * 3 + 1], state.light_color[state.active_count * 3 + 1]);
            std::swap(state.light_color[i * 3 + 2], state.light_color[state.active_count * 3 + 2]);

            std::swap(state.initial_light_color[i * 3 + 0], state.initial_light_color[state.active_count * 3 + 0]);
            std::swap(state.initial_light_color[i * 3 + 1], state.initial_light_color[state.active_count * 3 + 1]);
            std::swap(state.initial_light_color[i * 3 + 2], state.initial_light_color[state.active_count * 3 + 2]);

            std::swap(state.flags[i], state.flags[state.active_count]);
            std::swap(state.type[i], state.type[state.active_count]);
            std::swap(state.variant[i], state.variant[state.active_count]);
        }

        i++;
    }

    for (size_t i = 0; i < state.active_count; ++i) {
        const float lifetime = state.lifetime[i];
        const float max_lifetime = state.max_lifetime[i];
        const float custom_scale = state.custom_scale[i];
        float& scale = state.scale[i];

        const float s = std::clamp(lifetime / (max_lifetime * 0.5f), 0.0f, 1.0f);

        scale = custom_scale * s;

        // Dim light
        state.light_color[i * 3 + 0] = state.initial_light_color[i * 3 + 0] * s;
        state.light_color[i * 3 + 1] = state.initial_light_color[i * 3 + 1] * s;
        state.light_color[i * 3 + 2] = state.initial_light_color[i * 3 + 2] * s;
    }
}

void ParticleManager::Terminate() {
#if defined(__AVX__)
    ALIGNED_FREE(state.position);
    ALIGNED_FREE(state.velocity);
    ALIGNED_FREE(state.rotation);
    ALIGNED_FREE(state.rotation_speed);
    ALIGNED_FREE(state.lifetime);
#else
    free(state.position);
    free(state.velocity);
    free(state.rotation);
    free(state.rotation_speed);
    free(state.lifetime);
#endif

    free(state.light_color);
    free(state.max_lifetime);
    free(state.custom_scale);
    free(state.scale);
    free(state.flags);
    free(state.type);
    free(state.variant);
}