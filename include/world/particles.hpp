#pragma once

#ifndef PARTICLES_HPP
#define PARTICLES_HPP

#include <stdint.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "time/time.hpp"
#include "types/block.hpp"

constexpr size_t MAX_PARTICLES_COUNT = 10000;
constexpr float PARTICLE_SIZE = 8.0f;

namespace Particle {

    enum class Type : uint8_t {
        Dirt = 0,
        Stone = 1,
        Grass = 2,
        Wood = 7
    };

    inline constexpr Particle::Type get_by_block(BlockType block_type) {
        switch (block_type) {
        case BlockType::Dirt: return Particle::Type::Dirt;
        case BlockType::Stone: return Particle::Type::Stone;
        case BlockType::Grass: return Particle::Type::Grass;
        case BlockType::Wood: return Particle::Type::Wood;
        }
    }

}

struct ParticleData {
    glm::vec2 position;
    glm::vec2 velocity;
    float spawn_time;
    float lifetime;
    float rotation_speed;
    float custom_scale;
    float scale;
    glm::quat rotation;
    bool gravity;
    Particle::Type type;
    uint8_t variant;
};

class ParticleBuilder {
private:
    ParticleBuilder() {}
public:
    static ParticleBuilder create(Particle::Type type, glm::vec2 position, glm::vec2 velocity, float lifetime) {
        ParticleBuilder builder;
        builder.position = position;
        builder.velocity = velocity;
        builder.scale = 1.0f;
        builder.lifetime = lifetime;
        builder.rotation_speed = 0.0f;
        builder.gravity = false;
        builder.type = type;
        builder.variant = static_cast<uint8_t>(rand() % 3);

        return builder;
    }

    ParticleBuilder& with_gravity(bool gravity) { this->gravity = gravity; return *this; }
    ParticleBuilder& with_rotation_speed(float speed) { this->rotation_speed = speed; return *this; }
    ParticleBuilder& with_scale(float scale) { this->scale = scale; return *this; }

    ParticleData build(void) const {
        return ParticleData {
            .position       = this->position,
            .velocity       = this->velocity,
            .spawn_time     = Time::elapsed_seconds(),
            .lifetime       = this->lifetime,
            .rotation_speed = this->rotation_speed,
            .custom_scale   = this->scale,
            .scale          = 1.0f,
            .rotation       = glm::mat4(1.0f),
            .gravity        = this->gravity,
            .type           = this->type,
            .variant        = this->variant
        };
    }

private:
    glm::vec2 position;
    glm::vec2 velocity;
    float scale;
    float lifetime;
    float rotation_speed;
    bool gravity;
    Particle::Type type;
    uint8_t variant;
};

class ParticleManager {
    ParticleManager() = delete;
    ParticleManager(ParticleManager&) = delete;
    ParticleManager(ParticleManager&&) = delete;
    ParticleManager& operator=(ParticleManager &) = delete;
    ParticleManager& operator=(ParticleManager &&) = delete;

public:
    static void init() {
        particles.reserve(MAX_PARTICLES_COUNT);
    }

    static void render(void);
    static void fixed_update(void);

    static void spawn_particle(const ParticleBuilder& builder) {
        particles[particles_index] = builder.build();
        particles_index = ++particles_index % MAX_PARTICLES_COUNT;
    }

private:
    static std::unordered_map<size_t, ParticleData> particles;
    static size_t particles_index;
};

#endif