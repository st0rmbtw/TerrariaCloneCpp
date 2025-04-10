#ifndef WORLD_PARTICLES_HPP
#define WORLD_PARTICLES_HPP

#pragma once

#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <SGE/math/quat.hpp>

#include "world/world.hpp"
#include "types/block.hpp"

namespace Particle {
    enum class Type : uint8_t {
        Dirt = 0,
        Stone = 1,
        Grass = 2,
        Torch = 6,
        Wood = 7
    };

    inline constexpr Particle::Type get_by_tile(TileType tile_type) {
        switch (tile_type) {
        case TileType::Dirt: return Particle::Type::Dirt;
        case TileType::Stone: return Particle::Type::Stone;
        case TileType::Grass: return Particle::Type::Grass;
        case TileType::Wood: return Particle::Type::Wood;
        case TileType::Torch: return Particle::Type::Torch;
        }
    }

    inline constexpr Particle::Type get_by_wall(WallType wall_type) {
        switch (wall_type) {
        case WallType::DirtWall: return Particle::Type::Dirt;
        case WallType::StoneWall: return Particle::Type::Stone;
        case WallType::WoodWall: return Particle::Type::Wood;
        }
    }
}

struct ParticleData {
    glm::quat rotation;
    glm::quat rotation_speed;
    std::optional<glm::vec3> light_color;
    glm::vec2 position;
    glm::vec2 velocity;
    float lifetime;
    float custom_scale;
    float scale;
    bool gravity;
    bool is_world;
    Particle::Type type;
    uint8_t variant;
};

class ParticleBuilder {
private:
    explicit ParticleBuilder(Particle::Type type) :
        m_position{},
        m_velocity{},
        m_type(type) {}

    explicit ParticleBuilder(Particle::Type type, glm::vec2 position, glm::vec2 velocity, float scale, float lifetime, float rotation_speed, bool gravity, uint8_t variant) :
        m_position(position),
        m_velocity(velocity),
        m_scale(scale),
        m_lifetime(lifetime),
        m_rotation_speed(rotation_speed),
        m_gravity(gravity),
        m_variant(variant),
        m_type(type) {}
public:
    static ParticleBuilder create(Particle::Type type, glm::vec2 position, glm::vec2 velocity, float lifetime) {
        return ParticleBuilder(type, position, velocity, 1.0f, lifetime, 0.0f, false, static_cast<uint8_t>(rand() % 3));
    }

    ParticleBuilder& with_gravity(bool gravity) { m_gravity = gravity; return *this; }
    ParticleBuilder& with_rotation_speed(float speed) { m_rotation_speed = speed; return *this; }
    ParticleBuilder& with_scale(float scale) { m_scale = scale; return *this; }
    ParticleBuilder& with_light(glm::vec3 light_color) { m_light_color = light_color; return *this; }
    ParticleBuilder& in_world_layer() { m_is_world = true; return *this; }

    [[nodiscard]] 
    ParticleData build() const {
        return ParticleData {
            .rotation       = glm::mat4(1.0f),
            .rotation_speed = Quat::from_rotation_z(m_rotation_speed),
            .light_color    = m_light_color,
            .position       = m_position,
            .velocity       = m_velocity,
            .lifetime       = m_lifetime,
            .custom_scale   = m_scale,
            .scale          = m_scale,
            .gravity        = m_gravity,
            .is_world       = m_is_world,
            .type           = m_type,
            .variant        = m_variant
        };
    }

private:
    std::optional<glm::vec3> m_light_color;
    glm::vec2 m_position;
    glm::vec2 m_velocity;
    float m_scale = 1.0f;
    float m_lifetime = 0.0f;
    float m_rotation_speed = 0.0f;
    bool m_gravity = false;
    bool m_is_world = false;
    uint8_t m_variant = 0;
    Particle::Type m_type;
};

namespace ParticleManager {
    void Init();

    void Draw();
    void Update(World& world);
    void DeleteExpired();

    void SpawnParticle(const ParticleBuilder& builder);

    void Terminate();
};

#endif