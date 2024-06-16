#pragma once

#include "optional.hpp"
#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <glm/glm.hpp>
#include "common.h"

#include "input.hpp"
#include "render/camera.h"
#include "render/texture_atlas_sprite.hpp"
#include "types/sprite.hpp"
#include "world/world.hpp"
#include "time/timer.hpp"
#include "player/inventory.hpp"

constexpr float PLAYER_WIDTH = 22.0f;
constexpr float PLAYER_WIDTH_HALF = PLAYER_WIDTH / 2.0f;
constexpr float PLAYER_HEIGHT = 42.0f;
constexpr float PLAYER_HEIGHT_HALF = PLAYER_HEIGHT / 2.0f;

constexpr float GRAVITY = 0.4f;
constexpr float ACCELERATION = 0.1f;
constexpr float SLOWDOWN = 0.2f;

constexpr uint32_t JUMP_HEIGHT = 15;
constexpr float JUMP_SPEED = 5.01;

constexpr float MAX_WALK_SPEED = 3.0;
constexpr float MAX_FALL_SPEED = 10.0;

const size_t WALK_ANIMATION_LENGTH = 13;

static const glm::vec2 ITEM_ANIMATION_POINTS[] = {
    glm::vec2(-7.5, -11.0), glm::vec2(6.0, -7.5), glm::vec2(7.0, 4.0)
};

const float ITEM_ROTATION = 1.7;

enum class Direction : uint8_t {
    Left = 0,
    Right = 1
};

enum class MovementState : uint8_t {
    Idle = 0,
    Walking = 1,
    Flying = 2
};

struct WalkAnimation {
    unsigned char offset = 0;
    unsigned char length = WALK_ANIMATION_LENGTH;
};

struct FlyAnimation {
    unsigned char index = 0;
};

struct IdleAnimation {
    unsigned char index = 0;
};

struct UseItemAnimation {
    unsigned char offset = 0;
};

struct PlayerSprite {
    TextureAtlasSprite sprite;
    WalkAnimation walk_animation;
    FlyAnimation fly_animation;
    IdleAnimation idle_animation;
    UseItemAnimation use_item_animation;

    PlayerSprite(const TextureAtlasSprite& sprite, WalkAnimation walk_anim = {}, FlyAnimation fly_anim = {}, IdleAnimation idle_anim = {}, UseItemAnimation use_item_anim = {}) :
        sprite(sprite),
        walk_animation(walk_anim),
        fly_animation(fly_anim),
        idle_animation(idle_anim),
        use_item_animation(use_item_anim) {}

    inline void set_walk_animation(WalkAnimation walk_anim) {
        this->walk_animation = walk_anim;
    }

    inline void set_fly_animation(FlyAnimation fly_anim) {
        this->fly_animation = fly_anim;
    }

    inline void set_idle_animation(IdleAnimation idle_anim) {
        this->idle_animation = idle_anim;
    }

    inline void set_use_item_animation(UseItemAnimation use_item_anim) {
        this->use_item_animation = use_item_anim;
    }
};

struct Collisions {
    bool up;
    bool down;
    bool left;
    bool right;
};

class Player {
public:
    Player();

    void pre_update();
    void fixed_update(const delta_time_t& delta_time, const World& world);
    void update(const Camera& camera, World& world);
    void render(const Camera& camera) const;

    void set_position(const World& world, const glm::vec2& position);

    const glm::vec2& position(void) const { return m_position; }
    Direction direction(void) const { return m_direction; }

    const Inventory& inventory(void) const { return m_inventory; }
    Inventory& inventory(void) { return m_inventory; }
    
private:
    void horizontal_movement();
    void vertical_movement();
    void gravity(void);
    glm::vec2 check_collisions(const World& world);
    void keep_in_world_bounds(const World& world);
    void update_sprites(void);
    void update_walk_anim_timer(void);
    void update_sprites_index(const delta_time_t& delta_time);
    void update_movement_state(void);
    void update_using_item_anim(void);
    void spawn_particles_on_walk(void) const;
    void spawn_particles_grounded(void) const;

    inline float get_fall_distance(void) const;

    void use_item(const Camera& camera, World& world);

private:
    glm::vec2 m_position;
    glm::vec2 m_velocity;
    Direction m_direction = Direction::Right;
    size_t m_walk_animation_index = 0;
    Timer m_walk_anim_timer;
    Timer m_walk_particles_timer;
    Collisions m_collisions;
    int m_jump = 0;
    float m_fall_start = -1;
    bool m_jumping = false;
    bool do_jump = false;
    int m_use_cooldown = 0;
    int m_swing_counter = 0;
    int m_swing_counter_max = 0;
    int m_swing_anim_index = 0;
    bool m_swing_anim = false;
    bool m_using_item_visible = false;
    bool m_prev_grounded = false;
    tl::optional<BlockType> m_stand_on_block = tl::nullopt;

    MovementState m_movement_state;

    PlayerSprite hair;
    PlayerSprite head;
    PlayerSprite body;
    PlayerSprite feet;
    PlayerSprite left_hand;
    PlayerSprite left_shoulder;
    PlayerSprite right_arm;
    PlayerSprite left_eye;
    PlayerSprite right_eye;
    Sprite using_item;

    Inventory m_inventory;
};

#endif