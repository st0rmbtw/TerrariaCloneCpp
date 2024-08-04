#include "particles.hpp"

#include "../math/math.hpp"
#include "../math/quat.hpp"
#include "../renderer/renderer.hpp"
#include "../types/sprite.hpp"
#include "../common.h"
#include "../assets.hpp"

inline constexpr size_t get_particle_index(Particle::Type type, uint8_t variant) {
    ASSERT(variant <= 2, "Variant must be in range from 0 to 3");

    const size_t index = static_cast<uint8_t>(type);
    const size_t y = index / PARTICLES_ATLAS_COLUMNS;
    const size_t x = index % PARTICLES_ATLAS_COLUMNS;
    return (y * 3 + variant) * PARTICLES_ATLAS_COLUMNS + x;
}

static struct ParticlesState {
    std::vector<ParticleData> particles;
    size_t particles_index = 0;
} state;

void ParticleManager::Init() {
    state.particles.reserve(MAX_PARTICLES_COUNT);
}

void ParticleManager::SpawnParticle(const ParticleBuilder& builder) {
    if (state.particles.size() < MAX_PARTICLES_COUNT) {
        state.particles.push_back(builder.build());
    } else {
        state.particles[state.particles_index] = builder.build();
        state.particles_index = ++state.particles_index % MAX_PARTICLES_COUNT;
    }
}

void ParticleManager::Render() {
    if (state.particles.empty()) return;

    TextureAtlasSprite sprite(Assets::GetTextureAtlas(AssetKey::TextureParticles));

    for (const ParticleData& particle : state.particles) {
        sprite.set_position(particle.position);
        sprite.set_scale(glm::vec2(particle.scale));
        sprite.set_index(get_particle_index(particle.type, particle.variant));
        sprite.set_rotation(particle.rotation);

        Renderer::DrawAtlasSprite(sprite, RenderLayer::World);
    }
}

void ParticleManager::Update() {
    for (ParticleData& particle : state.particles) {
        if (particle.gravity) {
            particle.velocity.y += 0.1f;
        }

        particle.position += particle.velocity;
        particle.rotation *= Quat::from_rotation_z(particle.rotation_speed);
    }
}

void ParticleManager::DeleteExpired() {
    for (auto it = state.particles.begin(); it != state.particles.end();) {
        ParticleData& particle = *it;

        if (Time::elapsed_seconds() >= particle.spawn_time + particle.lifetime) {
            it = state.particles.erase(it);
            continue;
        }

        const float scale = glm::clamp(
            0.0f, 1.0f, 
            map_range(
                particle.spawn_time + particle.lifetime * 0.5f, particle.spawn_time + particle.lifetime,
                1.0f, 0.0f,
                Time::elapsed_seconds()
            )
        );

        particle.scale = particle.custom_scale * scale;

        it++;
    }
}