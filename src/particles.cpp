#include "particles.hpp"

#include <xmmintrin.h>

#include "math/math.hpp"
#include "renderer/renderer.hpp"

#ifdef _WIN32
#include <corecrt_malloc.h>
#define ALIGNED_ALLOC(size, alignment) _aligned_malloc((size), (alignment))
#define ALIGNED_FREE(ptr) _aligned_free((ptr))
#else
#include <stdlib.h>
#define ALIGNED_ALLOC(size, alignment) aligned_alloc((alignment), (size))
#define ALIGNED_FREE(ptr) free((ptr))
#endif

static struct ParticlesState {
    float* position;
    float* velocity;
    float* spawn_time;
    float* lifetime;
    float* custom_scale;
    float* scale;
    float* rotation;
    float* rotation_speed;
    bool* gravity;
    bool* active;
    Particle::Type* type;
    uint8_t* variant;

    size_t active_count = 0;
} state;

void ParticleManager::Init() {
    state.position = (float*) ALIGNED_ALLOC(MAX_PARTICLES_COUNT * 2 * sizeof(float), 32);
    state.velocity = (float*) ALIGNED_ALLOC(MAX_PARTICLES_COUNT * 2 * sizeof(float), 32);
    state.spawn_time = new float[MAX_PARTICLES_COUNT];
    state.lifetime = new float[MAX_PARTICLES_COUNT];
    state.rotation_speed = new float[MAX_PARTICLES_COUNT];
    state.custom_scale = new float[MAX_PARTICLES_COUNT];
    state.scale = new float[MAX_PARTICLES_COUNT];
    state.rotation = (float*) ALIGNED_ALLOC(MAX_PARTICLES_COUNT * 4 * sizeof(float), 32);
    state.rotation_speed = (float*) ALIGNED_ALLOC(MAX_PARTICLES_COUNT * 4 * sizeof(float), 32);
    state.gravity = new bool[MAX_PARTICLES_COUNT];
    state.active = new bool[MAX_PARTICLES_COUNT](); // If it's not initilized with false, particles wouldn't spawn
    state.type = new Particle::Type[MAX_PARTICLES_COUNT];
    state.variant = new uint8_t[MAX_PARTICLES_COUNT];
}

void ParticleManager::SpawnParticle(const ParticleBuilder& builder) {
    const uint32_t index = state.active_count;
    ParticleData particle_data = builder.build();

    state.position[index * 2 + 0] = particle_data.position.x;
    state.position[index * 2 + 1] = particle_data.position.y;

    state.velocity[index * 2 + 0] = particle_data.velocity.x;
    state.velocity[index * 2 + 1] = particle_data.velocity.y;

    state.spawn_time[index] = particle_data.spawn_time;
    state.lifetime[index] = particle_data.lifetime;
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

    state.gravity[index] = particle_data.gravity;
    state.active[index] = true;
    state.type[index] = particle_data.type;
    state.variant[index] = particle_data.variant;

    state.active_count = ++state.active_count % MAX_PARTICLES_COUNT;
}

void ParticleManager::Render() {
    for (size_t i = 0; i < state.active_count; ++i) {
        glm::vec2 position;
        position.x = state.position[i * 2 + 0];
        position.y = state.position[i * 2 + 1];

        const __m128 rotation = _mm_load_ps(&state.rotation[i * 4]);

        const float scale = state.scale[i];
        const Particle::Type type = state.type[i];
        const uint8_t variant = state.variant[i];

        Renderer::DrawParticle(position, *reinterpret_cast<const glm::quat*>(&rotation), scale, type, variant);
    }
}

void ParticleManager::Update() {
    for (size_t i = 0; i < state.active_count; ++i) {
        const bool gravity = state.gravity[i];
        float& velocity_y = state.velocity[i * 2 + 1];

        if (gravity) {
            velocity_y += 0.1f;
        }
    }
    
    // A vec2 type is the size of 64 bit, allowing us to pack 4 into 256 bit vector.
    for (size_t i = 0; i < state.active_count * 2; i += 8) {
        const __m256 positionVector = _mm256_load_ps(&state.position[i]);
        const __m256 velocityVector = _mm256_load_ps(&state.velocity[i]);
        const __m256 newPosition = _mm256_add_ps(positionVector, velocityVector);
        _mm256_storeu_ps(&state.position[i], newPosition);
    }

    // A quat type is the size of 128 bit, allowing us to pack 2 into 256 bit vector.
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
		_mm256_storeu_ps(&state.rotation[i], _mm256_shuffle_ps(XZWY, XZWY, _MM_SHUFFLE(2,1,3,0)));
    }
}

void ParticleManager::DeleteExpired() {
    size_t i = 0;

    while (state.active_count > 0) {
        const float spawn_time = state.spawn_time[i];
        const float lifetime = state.lifetime[i];
        bool& active = state.active[i];
        if (!active) break;

        if (Time::elapsed_seconds() >= spawn_time + lifetime) {
            state.active_count--;
            active = false;
            std::swap(state.position[i * 2], state.position[state.active_count * 2]);
            std::swap(state.position[i * 2 + 1], state.position[state.active_count * 2 + 1]);

            std::swap(state.velocity[i * 2], state.velocity[state.active_count * 2]);
            std::swap(state.velocity[i * 2 + 1], state.velocity[state.active_count * 2 + 1]);

            std::swap(state.spawn_time[i], state.spawn_time[state.active_count]);
            std::swap(state.lifetime[i], state.lifetime[state.active_count]);
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

            std::swap(state.gravity[i], state.gravity[state.active_count]);
            std::swap(state.active[i], state.active[state.active_count]);
            std::swap(state.type[i], state.type[state.active_count]);
            std::swap(state.variant[i], state.variant[state.active_count]);
        }

        i++;
    }

    for (size_t i = 0; i < state.active_count; ++i) {
        const float spawn_time = state.spawn_time[i];
        const float lifetime = state.lifetime[i];
        const float custom_scale = state.custom_scale[i];
        float& scale = state.scale[i];

        const float s = glm::clamp(
            0.0f, 1.0f, 
            map_range(
                spawn_time + lifetime * 0.5f, spawn_time + lifetime,
                1.0f, 0.0f,
                Time::elapsed_seconds()
            )
        );

        scale = custom_scale * s;
    }
}

void ParticleManager::Terminate() {
    ALIGNED_FREE(state.position);
    ALIGNED_FREE(state.velocity);
    ALIGNED_FREE(state.rotation);
    ALIGNED_FREE(state.rotation_speed);
}