#include "world/particles.hpp"
#include "common.h"
#include "assets.hpp"
#include "math/math.hpp"
#include "math/quat.hpp"
#include "renderer/renderer.hpp"
#include "types/sprite.hpp"

inline size_t get_particle_index(Particle::Type type, uint8_t variant) {
    ASSERT(variant <= 2, "Variant must be in range of 0 to 3");

    size_t index = static_cast<uint8_t>(type);
    size_t y = index / PARTICLES_ATLAS_COLUMNS;
    size_t x = index % PARTICLES_ATLAS_COLUMNS;
    return (y * 3 + variant) * PARTICLES_ATLAS_COLUMNS + x;
}

std::unordered_map<size_t, ParticleData> ParticleManager::particles = {};
size_t ParticleManager::particles_index = 0;

void ParticleManager::render(void) {
    for (const auto& entry : ParticleManager::particles) {
        const ParticleData& particle = entry.second;

        TextureAtlasSprite sprite(Assets::GetTextureAtlas(AssetKey::TextureParticles));
        sprite.set_position(particle.position);
        sprite.set_index(get_particle_index(particle.type, particle.variant));
        sprite.set_rotation(particle.rotation);
        sprite.set_scale(glm::vec2(particle.scale));

        Renderer::DrawAtlasSprite(sprite, RenderLayer::World);
    }
}

void ParticleManager::fixed_update(void) {
    for (auto it = ParticleManager::particles.begin(); it != ParticleManager::particles.end();) {
        ParticleData& particle = it->second;

        if (Time::elapsed_seconds() >= particle.spawn_time + particle.lifetime) {
            it = ParticleManager::particles.erase(it);
            continue;
        }
        
        if (particle.gravity) {
            particle.velocity.y += 0.1f;
        }

        particle.position += particle.velocity;
        particle.rotation *= Quat::from_rotation_z(particle.rotation_speed);

        float scale = glm::clamp(
            0.0f, 1.0f, 
            map_range(
                particle.spawn_time + particle.lifetime / 2.0f, particle.spawn_time + particle.lifetime,
                1.0f, 0.0f,
                Time::elapsed_seconds()
            )
        );

        particle.scale = particle.custom_scale * scale;

        it++;
    }
}