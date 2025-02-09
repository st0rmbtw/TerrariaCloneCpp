#pragma once

#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "inventory.hpp"

#include <optional>
#include <glm/glm.hpp>

#include "../engine/renderer/camera.hpp"
#include "../engine/types/sprite.hpp"
#include "../engine/time/timer.hpp"

#include "../world/world.hpp"

static constexpr size_t WALK_ANIMATION_LENGTH = 13;

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
    uint8_t offset = 0;
    uint8_t length = WALK_ANIMATION_LENGTH;
};

struct FlyAnimation {
    uint8_t index = 0;
};

struct IdleAnimation {
    uint8_t index = 0;
};

struct UseItemAnimation {
    uint8_t offset = 0;
};

struct PlayerSprite {
    TextureAtlasSprite sprite;
    WalkAnimation walk_animation;
    FlyAnimation fly_animation;
    IdleAnimation idle_animation;
    UseItemAnimation use_item_animation;

    PlayerSprite() :
        walk_animation(),
        fly_animation(),
        idle_animation(),
        use_item_animation() {}

    PlayerSprite(TextureAtlasSprite sprite, WalkAnimation walk_anim = {}, FlyAnimation fly_anim = {}, IdleAnimation idle_anim = {}, UseItemAnimation use_item_anim = {}) :
        sprite(std::move(sprite)),
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
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
};

class Player {
public:
    void init();

    void pre_update();
    void fixed_update(const World& world, bool handle_input);
    void update(const Camera& camera, World& world);
    void draw() const;

    void set_position(const World& world, const glm::vec2& position);

    [[nodiscard]]
    inline glm::vec2 draw_position() const noexcept {
        return glm::vec2(m_position.x, m_position.y + m_draw_offset_y);
    }

    [[nodiscard]] const glm::vec2& position() const { return m_position; }
    [[nodiscard]] Direction direction() const { return m_direction; }

    [[nodiscard]] const Inventory& inventory() const { return m_inventory; }
    [[nodiscard]] Inventory& inventory() { return m_inventory; }

    [[nodiscard]] constexpr bool can_use_item() const { return true; }
    
private:
    void horizontal_movement(bool handle_input);
    void vertical_movement(bool handle_input);
    void gravity();
    glm::vec2 check_collisions(const World& world);
    void keep_in_world_bounds(const World& world);
    void update_sprites();
    void update_walk_anim_timer();
    void update_sprites_index();
    void update_movement_state();
    void update_using_item_anim();
    void update_hold_item();
    void spawn_particles_on_walk() const;
    void spawn_particles_grounded() const;

    [[nodiscard]] float get_fall_distance() const;

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
    float m_step_speed = 1.0f;
    float m_draw_offset_y = 0.0f;
    bool m_jumping = false;
    bool do_jump = false;
    int m_use_cooldown = 0;
    int m_swing_counter = 0;
    int m_swing_counter_max = 0;
    int m_swing_anim_index = 0;
    bool m_swing_anim = false;
    bool m_using_item_visible = false;
    bool m_prev_grounded = false;
    std::optional<BlockType> m_stand_on_block = std::nullopt;

    MovementState m_movement_state;

    PlayerSprite m_hair;
    PlayerSprite m_head;
    PlayerSprite m_body;
    PlayerSprite m_legs;
    PlayerSprite m_left_hand;
    PlayerSprite m_left_shoulder;
    PlayerSprite m_right_arm;
    PlayerSprite m_left_eye;
    PlayerSprite m_right_eye;
    Sprite m_using_item;

    Inventory m_inventory;
};

#endif