#include "player.hpp"

#include <glm/gtc/random.hpp>

#include <tracy/Tracy.hpp>

#include "../engine/math/quat.hpp"
#include "../engine/math/rect.hpp"
#include "../engine/input.hpp"
#include "../engine/time/time.hpp"
#include "../engine/time/timer.hpp"

#include "../assets.hpp"
#include "../constants.hpp"
#include "../types/item.hpp"
#include "../types/block.hpp"
#include "../renderer/renderer.hpp"
#include "../math/math.hpp"
#include "../utils.hpp"
#include "../particles.hpp"

using Constants::TILE_SIZE;

static constexpr float PLAYER_WIDTH = 20.0f;
static constexpr float PLAYER_HEIGHT = 42.0f;
static constexpr float PLAYER_WIDTH_HALF = PLAYER_WIDTH / 2.0f;
static constexpr float PLAYER_HEIGHT_HALF = PLAYER_HEIGHT / 2.0f;

static constexpr float GRAVITY = 0.4f;
static constexpr float ACCELERATION = 0.1f;
static constexpr float SLOWDOWN = 0.2f;

static constexpr uint32_t JUMP_HEIGHT = 15;
static constexpr float JUMP_SPEED = 5.01;

static constexpr float MAX_WALK_SPEED = 3.0;
static constexpr float MAX_FALL_SPEED = 10.0;

static const glm::vec2 ITEM_ANIMATION_POINTS[] = {
    glm::vec2(-7.5f, -11.0f), glm::vec2(6.0f, -7.5f), glm::vec2(7.0f, 4.0f)
};

static const glm::vec2 ITEM_HOLD_POINTS[] = {
    glm::vec2(7.0f, 4.0f) // HoldFront
};

static constexpr float ITEM_ROTATION = 1.7;

void spawn_particles_on_dig(const glm::vec2& position, Particle::Type particle, bool broken) {
    const float rotation_speed = glm::pi<float>() / 12.0f;

    const int count = broken ? rand_range(7, 15) : rand_range(3, 8);
    
    for (int i = 0; i < count; i++) {
        const glm::vec2 velocity = glm::normalize(glm::diskRand(1.25f));
        const float min_scale = broken ? 0.6f : 0.3f;
        const float scale = glm::linearRand(min_scale, 1.0f);

        ParticleManager::SpawnParticle(
            ParticleBuilder::create(particle, position, velocity, 1.25f)
                .in_world_layer()
                .with_gravity(true)
                .with_rotation_speed(rotation_speed)
                .with_scale(scale)
        );
    }
}

void Player::init() {
    ZoneScopedN("Player::init");

    m_hair = TextureAtlasSprite(Assets::GetTextureAtlas(TextureAsset::PlayerHair));
    m_head = TextureAtlasSprite(Assets::GetTextureAtlas(TextureAsset::PlayerHead));
    m_body = TextureAtlasSprite(Assets::GetTextureAtlas(TextureAsset::PlayerChest));
    m_legs = TextureAtlasSprite(Assets::GetTextureAtlas(TextureAsset::PlayerLegs));
    m_left_hand = TextureAtlasSprite(Assets::GetTextureAtlas(TextureAsset::PlayerLeftHand));
    m_left_shoulder = TextureAtlasSprite(Assets::GetTextureAtlas(TextureAsset::PlayerLeftShoulder));
    m_right_arm = TextureAtlasSprite(Assets::GetTextureAtlas(TextureAsset::PlayerRightArm));
    m_left_eye = TextureAtlasSprite(Assets::GetTextureAtlas(TextureAsset::PlayerLeftEye));
    m_right_eye = TextureAtlasSprite(Assets::GetTextureAtlas(TextureAsset::PlayerRightEye));
    m_walk_anim_timer = Timer::zero(TimerMode::Repeating);
    m_walk_particles_timer = Timer::from_seconds(1.0f / 20.0f, TimerMode::Repeating);
    m_walk_animation_index = 0;

    m_legs.set_walk_animation({ .offset = 6 });
    m_legs.set_fly_animation({ .index = 5 });

    m_left_shoulder.set_walk_animation({ .offset = 13 });
    m_left_shoulder.set_fly_animation({ .index = 2 });
    m_left_shoulder.set_use_item_animation({ .offset = 2 });

    m_left_hand.set_walk_animation({ .offset = 13 });
    m_left_hand.set_fly_animation({ .index = 2 });
    m_left_hand.set_use_item_animation({ .offset = 2 });

    m_right_arm.set_fly_animation({ .index = 13 });
    m_right_arm.set_idle_animation({ .index = 14 });
    m_right_arm.set_use_item_animation({ .offset = 15 });

    m_left_eye.set_walk_animation({ .offset = 6, .length = 14 });
    m_right_eye.set_walk_animation({ .offset = 6, .length = 14 });

    m_head.sprite
        .set_color(glm::vec4(0.92, 0.45, 0.32, 1.0));
    m_right_arm.sprite
        .set_color(glm::vec4(0.92, 0.45, 0.32, 1.0));
    m_right_eye.sprite
        .set_color(glm::vec4(89.0 / 255.0, 76.0 / 255.0, 64.0 / 255.0, 1.0));
    m_hair.sprite
        .set_color(glm::vec4(0.55, 0.23, 0.14, 1.0));
    m_body.sprite
        .set_color(glm::vec4(0.58, 0.55, 0.47, 1.0));
    m_legs.sprite
        .set_color(glm::vec4(190.0 / 255.0, 190.0 / 255.0, 156.0 / 255.0, 1.0));
    m_left_hand.sprite
        .set_color(glm::vec4(0.92, 0.45, 0.32, 1.0));
    m_left_shoulder.sprite
        .set_color(glm::vec4(0.58, 0.55, 0.47, 1.0));
}

void Player::set_position(const World& world, const glm::vec2& position) {
    m_position.x = position.x;
    m_position.y = position.y - PLAYER_HEIGHT_HALF;
    m_velocity.x = 0.0f;
    m_velocity.y = 0.0f;
    keep_in_world_bounds(world);
}

void Player::horizontal_movement(bool handle_input) {
    ZoneScopedN("Player::horizontal_movement");

    int8_t dir = 0;

    if (handle_input && Input::Pressed(Key::A)) { dir -= 1; }
    if (handle_input && Input::Pressed(Key::D)) { dir += 1; }

    if (dir > 0) {
        m_direction = Direction::Right;
        if (m_velocity.x < 0) {
            m_velocity.x *= 0.9;
        }
        m_velocity.x += ACCELERATION;
    } else if (dir < 0) {
        m_direction = Direction::Left;
        if (m_velocity.x > 0) {
            m_velocity.x *= 0.9;
        }
        m_velocity.x -= ACCELERATION;
    } else {
        m_velocity.x = move_towards(m_velocity.x, 0, SLOWDOWN);
    }

    m_velocity.x = glm::clamp(m_velocity.x, -MAX_WALK_SPEED, MAX_WALK_SPEED);
}

void Player::vertical_movement(bool handle_input) {
    ZoneScopedN("Player::vertical_movement");

    if (do_jump && m_collisions.down) {
        m_jump = JUMP_HEIGHT;
        m_velocity.y = -JUMP_SPEED;
        m_jumping = true;
    }

    if (handle_input && Input::Pressed(Key::Space)) {
        if (m_jump > 0) {
            if (m_velocity.y == 0) {
                m_jump = 0;
            } else {
                m_velocity.y = -JUMP_SPEED;
                m_jump -= 1;
            }
        }
    } else {
        m_jump = 0;
    }

    do_jump = false;
}

void Player::gravity() {
    ZoneScopedN("Player::gravity");

    if (!m_collisions.down && m_velocity.y > 0 && m_fall_start < 0) {
        m_fall_start = m_position.y;
    }

    m_velocity.y += GRAVITY;
    m_velocity.y = glm::clamp(m_velocity.y, -MAX_FALL_SPEED, MAX_FALL_SPEED);
}

glm::vec2 Player::check_collisions(const World& world) {
    ZoneScopedN("Player::check_collisions");

    glm::vec2 result = m_velocity;
    const glm::vec2 pos = m_position;
    const glm::vec2 next_pos = m_position + m_velocity;
    const math::IRect& area = world.playable_area();

    int left = static_cast<int>((m_position.x - PLAYER_WIDTH_HALF) / TILE_SIZE) - 1;
    int right = static_cast<int>((m_position.x + PLAYER_WIDTH_HALF) / TILE_SIZE) + 2;
    int top = static_cast<int>((m_position.y - PLAYER_HEIGHT_HALF) / TILE_SIZE) - 1;
    int bottom = static_cast<int>((m_position.y + PLAYER_HEIGHT_HALF) / TILE_SIZE) + 2;

    left = glm::clamp(left, area.min.x, area.max.x);
    right = glm::clamp(right, area.min.x, area.max.x);
    top = glm::clamp(top, area.min.y, area.max.y);
    bottom = glm::clamp(bottom, area.min.y, area.max.y);

    int hx = -1;
    int hy = -1;
    int vx = -1;
    int vy = -1;

    m_collisions = Collisions();

    for (int y = top; y < bottom; ++y) {
        for (int x = left; x < right; ++x) {
            if (!world.block_exists(TilePos(x, y))) continue;
            
            const glm::vec2 tile_pos = glm::vec2(x * TILE_SIZE, y * TILE_SIZE);

            if (
                next_pos.x + PLAYER_WIDTH_HALF > tile_pos.x && next_pos.x - PLAYER_WIDTH_HALF < tile_pos.x + TILE_SIZE &&
                next_pos.y + PLAYER_HEIGHT_HALF > tile_pos.y && next_pos.y - PLAYER_HEIGHT_HALF < tile_pos.y + TILE_SIZE
            ) {
                if (pos.y + PLAYER_HEIGHT_HALF <= tile_pos.y) {
                    vx = x;
                    vy = y;
                    if (vx != hx) {
                        m_collisions.down = true;
                        m_jumping = false;
                        m_stand_on_block = world.get_block_type(TilePos(x, y));

                        result.y = tile_pos.y - (pos.y + PLAYER_HEIGHT_HALF);
                    }
                } else if (pos.x + PLAYER_WIDTH_HALF <= tile_pos.x) {
                    hx = x;
                    hy = y;
                    if (hy != vy) {
                        result.x = tile_pos.x - (pos.x + PLAYER_WIDTH_HALF);
                        m_collisions.right = true;
                    }
                    if (vx == hx) {
                        result.y = m_velocity.y;
                    }
                } else if (pos.x - PLAYER_WIDTH_HALF >= tile_pos.x + TILE_SIZE) {
                    m_collisions.left = true;
                    hx = x;
                    hy = y;
                    if (hy != vy) {
                        result.x = tile_pos.x + TILE_SIZE - (pos.x - PLAYER_WIDTH_HALF);
                    }
                    if (vx == hx) {
                        result.y = m_velocity.y;
                    }
                } else if (pos.y - PLAYER_HEIGHT_HALF >= tile_pos.y + TILE_SIZE) {
                    m_collisions.up = true;

                    vx = x;
                    vy = y;
                    result.y = tile_pos.y + TILE_SIZE - (pos.y - PLAYER_HEIGHT_HALF);
                    if (vy == hy) {
                        result.x = m_velocity.x;
                    }
                }
            }
        }
    }

    left = static_cast<int>((m_position.x - PLAYER_WIDTH_HALF) / TILE_SIZE);
    right = static_cast<int>((m_position.x + PLAYER_WIDTH_HALF) / TILE_SIZE);
    bottom = static_cast<int>((m_position.y + PLAYER_HEIGHT_HALF + 4.0f) / TILE_SIZE);

    left = glm::clamp(left, area.min.x, area.max.x);
    right = glm::clamp(right, area.min.x, area.max.x);
    bottom = glm::clamp(bottom, area.min.y, area.max.y);

    float step_down_y;
    bool found_step_down_tile = false;

    if (result.y == GRAVITY) {
        const float pos_y = glm::floor((m_position.y + PLAYER_HEIGHT_HALF) / TILE_SIZE) * TILE_SIZE - PLAYER_HEIGHT_HALF;
        const int a = ((pos_y + PLAYER_HEIGHT_HALF + 4.0) / TILE_SIZE);
        const int b = PLAYER_HEIGHT_HALF / static_cast<int>(TILE_SIZE) + (static_cast<int>(PLAYER_HEIGHT_HALF) % 16 == 0 ? 0 : 1);
        const float c = ((a + b) * TILE_SIZE);

        for (int x = left; x <= right; ++x) {
            for (int y = bottom; y <= bottom + 1; ++y) {
                if (!world.block_exists(TilePos(x, y))) continue;

                const glm::vec2 tile_pos = glm::vec2(x * TILE_SIZE, y * TILE_SIZE);
                const math::Rect player_rect = math::Rect::from_center_half_size(m_position, glm::vec2(PLAYER_WIDTH_HALF, PLAYER_HEIGHT_HALF));
                const math::Rect tile_rect = math::Rect::from_top_left(glm::vec2(tile_pos.x, tile_pos.y - TILE_SIZE - 1.0f), glm::vec2(TILE_SIZE));

                if (player_rect.intersects(tile_rect) && tile_pos.y < c) {
                    step_down_y = tile_pos.y;
                    found_step_down_tile = true;
                    break;
                }
            }
        }
    }

    if (found_step_down_tile) {
        result.y = 0.0f;
        const float new_y = step_down_y - PLAYER_HEIGHT_HALF;
        m_draw_offset_y = m_position.y - new_y;
        m_position.y = new_y;
        m_step_speed = 2.0f;
    }

    const int a = glm::floor((m_position.y + PLAYER_HEIGHT_HALF - 1.0f) / TILE_SIZE);

    if (
        hy == a && (
            !world.block_exists(TilePos(hx, hy - 1)) &&
            !world.block_exists(TilePos(hx, hy - 2)) &&
            !world.block_exists(TilePos(hx, hy - 3))
        )
    ) {
        result.x = m_velocity.x;
        result.y = 0.0f;
        const float new_y = hy * 16.0f - PLAYER_HEIGHT_HALF;
        m_draw_offset_y = m_position.y - new_y;
        m_step_speed = 2.5f;
        m_position.y = new_y;
    }

    return result;
}

void Player::update_walk_anim_timer() {
    ZoneScopedN("Player::update_walk_anim_timer");

    if (m_velocity.x != 0.0) {
        const uint32_t time = glm::abs(100.0 / glm::abs(m_velocity.x));
        m_walk_anim_timer.set_duration(Duration::Millis(glm::max(time, 1u)));
    }
}

void Player::update_using_item_anim() {
    ZoneScopedN("Player::update_using_item_anim");

    if (m_use_cooldown > 0) m_use_cooldown -= 1;
    if (m_swing_counter > 0) m_swing_counter -= 1;

    if (m_swing_counter < m_swing_counter_max * 0.333) {
        m_swing_anim_index = 2;
    } else if (m_swing_counter < m_swing_counter_max * 0.666) {
        m_swing_anim_index = 1;
    } else {
        m_swing_anim_index = 0;
    }

    const float direction = m_direction == Direction::Right ? 1 : -1;

    glm::vec2 offset = ITEM_ANIMATION_POINTS[m_swing_anim_index];
    offset.x *= direction;

    const glm::vec2 item_position = draw_position() + offset;

    // 0..1
    float rotation = static_cast<float>(m_swing_counter) / static_cast<float>(m_swing_counter_max);
    // -1..1
    rotation = rotation * 2.0 - 1.;

    m_using_item.set_position(item_position);
    m_using_item.set_rotation(Quat::from_rotation_z(rotation * direction * ITEM_ROTATION + direction * 0.5f));
    m_using_item.set_anchor(m_direction == Direction::Left ? Anchor::BottomRight : Anchor::BottomLeft);
    m_using_item.set_flip_x(m_direction == Direction::Left);
    
    m_using_item_visible = true;
}

void Player::update_hold_item() {
    ZoneScopedN("Player::update_hold_item");

    const std::optional<Item>& selected_item = m_inventory.get_selected_item().item;
    if (!selected_item.has_value()) return;

    const HoldStyle hold_style = selected_item->hold_style;
    if (hold_style == HoldStyle::None) return;

    m_using_item.set_texture(Assets::GetItemTexture(selected_item->id));
    m_using_item.set_anchor(m_direction == Direction::Left ? Anchor::BottomRight : Anchor::BottomLeft);
    m_using_item.set_flip_x(m_direction == Direction::Left);
    m_using_item.set_rotation(glm::identity<glm::quat>());

    glm::vec2 offset;
    if (hold_style == HoldStyle::HoldFront) {
        offset = ITEM_HOLD_POINTS[0];
    }

    const float direction = m_direction == Direction::Right ? 1.0f : -1.0f;
    offset.x *= direction;

    const glm::vec2 item_position = draw_position() + offset;
    m_using_item.set_position(item_position);

    m_using_item_visible = true;
}

void Player::update_sprites_index() {
    ZoneScopedN("Player::update_sprites_index");

    PlayerSprite* sprites[] = { &m_hair, &m_head, &m_body, &m_legs, &m_left_hand, &m_left_shoulder, &m_right_arm, &m_left_eye, &m_right_eye };

    const std::optional<Item>& selected_item = m_inventory.get_selected_item().item;
    const HoldStyle hold_style = selected_item.has_value() ? selected_item->hold_style : HoldStyle::None;

    switch (m_movement_state) {
    case MovementState::Walking: {
        if (m_walk_anim_timer.tick(Time::fixed_delta()).just_finished()) {
            m_walk_animation_index = (m_walk_animation_index + 1) % WALK_ANIMATION_LENGTH;

            if (hold_style == HoldStyle::None) {
                for (PlayerSprite* sprite : sprites) {
                    const int index = sprite->walk_animation.offset + map_range(
                        0, WALK_ANIMATION_LENGTH,
                        0, sprite->walk_animation.length,
                        m_walk_animation_index
                    );
                    sprite->sprite.set_index(index);
                }
            } else {
                m_body.sprite.set_index(m_body.idle_animation.index);
                m_hair.sprite.set_index(m_hair.idle_animation.index);
                m_head.sprite.set_index(m_head.idle_animation.index);
                m_left_eye.sprite.set_index(m_left_eye.idle_animation.index);
                m_right_eye.sprite.set_index(m_right_eye.idle_animation.index);

                const int index = m_legs.walk_animation.offset + map_range(
                    0, WALK_ANIMATION_LENGTH,
                    0, m_legs.walk_animation.length,
                    m_walk_animation_index
                );
                m_legs.sprite.set_index(index);
            }
        }
    }
    break;
    case MovementState::Flying: {
        for (PlayerSprite* sprite : sprites) {
            sprite->sprite.set_index(sprite->fly_animation.index);
        }
    }
    break;
    case MovementState::Idle: {
        for (PlayerSprite* sprite : sprites) {
            sprite->sprite.set_index(sprite->idle_animation.index);
        }
    }
    break;
    }

    if (m_swing_anim) {
        PlayerSprite* use_item_sprites[] = { &m_left_hand, &m_left_shoulder, &m_right_arm };

        for (PlayerSprite* sprite : use_item_sprites) {
            sprite->sprite.set_index(sprite->use_item_animation.offset + m_swing_anim_index);
        }
    } else if (hold_style == HoldStyle::HoldFront) {
        PlayerSprite* use_item_sprites[] = { &m_left_hand, &m_left_shoulder, &m_right_arm };

        for (PlayerSprite* sprite : use_item_sprites) {
            sprite->sprite.set_index(sprite->use_item_animation.offset + 2);
        }
    }
}

void Player::update_movement_state() {
    ZoneScopedN("Player::update_movement_state");

    if (abs(m_velocity.y) > 0.0 || m_jumping) {
        m_movement_state = MovementState::Flying;
    } else if (m_velocity.x != 0) {
        m_movement_state = MovementState::Walking;
    } else {
        m_movement_state = MovementState::Idle;
    }
}

void Player::pre_update() {
    ZoneScopedN("Player::pre_update");

    if (Input::JustPressed(Key::Space)) {
        do_jump = true;
    }
}

void Player::spawn_particles_on_walk() const {
    ZoneScopedN("Player::spawn_particles_on_walk");

    if (m_movement_state != MovementState::Walking) return;
    if (!m_stand_on_block.has_value()) return;
    if (glm::abs(m_velocity.x) < 1.0f) return;

    const BlockType block = m_stand_on_block.value();
    if (!block_dusty(block)) return;

    const float direction = m_direction == Direction::Right ? -1.0f : 1.0f;

    const glm::vec2 position = draw_position() + glm::vec2(0., PLAYER_HEIGHT_HALF);
    const glm::vec2 velocity = random_point_cone(glm::vec2(direction, 0.0f), 45.0f);
    const float scale = rand_range(0.0f, 1.0f);
    const float rotation_speed = glm::pi<float>() / 12.0f;

    ParticleManager::SpawnParticle(
        ParticleBuilder::create(Particle::get_by_block(block), position, velocity, 0.3f)
            .in_world_layer()
            .with_scale(scale)
            .with_rotation_speed(rotation_speed)
    );
}

void Player::spawn_particles_grounded() const {
    ZoneScopedN("Player::spawn_particles_grounded");
    
    if (!m_stand_on_block.has_value()) return;
    const BlockType block = m_stand_on_block.value();

    if (!block_dusty(block)) return;

    const Particle::Type particle = Particle::get_by_block(block);

    const float fall_distance = get_fall_distance();
    const float rotation_speed = glm::pi<float>() / 12.0f;
    const glm::vec2 position = draw_position() + glm::vec2(0., PLAYER_HEIGHT_HALF);

    if (!m_prev_grounded && m_collisions.down && fall_distance > TILE_SIZE * 1.5) {
        for (int i = 0; i < 10; i++) {
            const float scale = rand_range(0.0f, 1.0f);
            const glm::vec2 point = random_point_circle(1.0f, 0.5f) * PLAYER_WIDTH_HALF;
            const glm::vec2 velocity = glm::vec2(glm::normalize(point).x, -0.5f);

            ParticleManager::SpawnParticle(
                ParticleBuilder::create(particle, position, velocity, 0.3f)
                    .in_world_layer()
                    .with_scale(scale)
                    .with_rotation_speed(rotation_speed)
            );
        }
    }
}

void Player::fixed_update(const World& world, bool handle_input) {
    ZoneScopedN("Player::fixed_update");

    m_using_item_visible = false;

    horizontal_movement(handle_input);
    vertical_movement(handle_input);
    gravity();

    const float a = 1.0f + abs(m_velocity.x) / MAX_WALK_SPEED;
    if (m_draw_offset_y > 0.0f) {
        m_draw_offset_y -= a * m_step_speed;
        if (m_draw_offset_y < 0.0f) m_draw_offset_y = 0.0f;
    } else if (m_draw_offset_y < 0.0f) {
        m_draw_offset_y += a * m_step_speed;
        if (m_draw_offset_y > 0.0f) m_draw_offset_y = 0.0f;
    }

    m_velocity = check_collisions(world);
    m_position += m_velocity;

    keep_in_world_bounds(world);

    update_movement_state();
    update_walk_anim_timer();

    if (m_swing_anim) {
        update_using_item_anim();
    } else {
        update_hold_item();
    }

    update_sprites_index();

    m_swing_anim_index = 2;
    if (m_swing_counter <= 0) {
        m_swing_anim = false;
    }

    if (m_walk_particles_timer.tick(Time::fixed_delta()).just_finished()) {
        spawn_particles_on_walk();
    }

    spawn_particles_grounded();

    m_prev_grounded = m_collisions.down;
    m_fall_start = -1;
}

void Player::update(const Camera& camera, World& world) {
    ZoneScopedN("Player::update");

    if (Input::Pressed(MouseButton::Left) && !Input::IsMouseOverUi()) {
        use_item(camera, world);
    }

    const std::optional<Item>& selected_item = m_inventory.get_selected_item().item;
    if (selected_item.has_value() && selected_item->id == ItemId::Torch) {
        const float direction = m_direction == Direction::Right ? 1.0f : -1.0f;
        const glm::vec2 size = m_using_item.size() - glm::vec2(2.0f * direction, 4.0f) * m_using_item.scale();

        world.add_light(Light {
            .color = glm::vec3(1.0f, 0.95f, 0.8f),
            .pos = get_lightmap_pos(m_using_item.position() + glm::vec2(size.x * direction, -size.y)),
            .size = glm::uvec2(2)
        });
    }

    update_sprites();
}

void Player::keep_in_world_bounds(const World& world) {
    ZoneScopedN("Player::keep_in_world_bounds");

    static constexpr float OFFSET = Constants::WORLD_BOUNDARY_OFFSET;

    const math::Rect area = world.playable_area() * TILE_SIZE;

    if (m_position.x - PLAYER_WIDTH_HALF < area.min.x + OFFSET) m_position.x = area.min.x + PLAYER_WIDTH_HALF + OFFSET;
    if (m_position.x + PLAYER_WIDTH_HALF > area.max.x - OFFSET) m_position.x = area.max.x - PLAYER_WIDTH_HALF - OFFSET;
    if (m_position.y - PLAYER_HEIGHT_HALF < area.min.y) m_position.y = area.min.y + PLAYER_HEIGHT_HALF;
    if (m_position.y + PLAYER_HEIGHT_HALF > area.max.y) m_position.y = area.max.y - PLAYER_HEIGHT_HALF;
}

float Player::get_fall_distance() const {
    if (m_fall_start < 0) return 0;
    return m_position.y + PLAYER_HEIGHT_HALF - m_fall_start;
}

void Player::update_sprites() {
    ZoneScopedN("Player::update_sprites");

    const bool flip_x = m_direction == Direction::Left;

    const glm::vec2 position = draw_position();

    m_hair.sprite
        .set_flip_x(flip_x)
        .set_position(position);
    m_head.sprite
        .set_flip_x(flip_x)
        .set_position(position);
    m_body.sprite
        .set_flip_x(flip_x)
        .set_position(position);
    m_legs.sprite
        .set_flip_x(flip_x)
        .set_position(position);
    m_left_hand.sprite
        .set_flip_x(flip_x)
        .set_position(position);
    m_left_shoulder.sprite
        .set_flip_x(flip_x)
        .set_position(position);
    m_right_arm.sprite
        .set_flip_x(flip_x)
        .set_position(position);
    m_left_eye.sprite
        .set_flip_x(flip_x)
        .set_position(position);
    m_right_eye.sprite
        .set_flip_x(flip_x)
        .set_position(position);
}

void Player::draw() const {
    ZoneScopedN("Player::draw");

    GameRenderer::DrawAtlasSpriteWorld(m_head.sprite);

    GameRenderer::DrawAtlasSpriteWorld(m_right_arm.sprite);

    GameRenderer::DrawAtlasSpriteWorld(m_hair.sprite);
    GameRenderer::DrawAtlasSpriteWorld(m_body.sprite);
    GameRenderer::DrawAtlasSpriteWorld(m_legs.sprite);

    GameRenderer::BeginOrderMode();
        GameRenderer::DrawAtlasSpriteWorld(m_left_eye.sprite);
        GameRenderer::DrawAtlasSpriteWorld(m_right_eye.sprite);
    GameRenderer::EndOrderMode();

    if (m_using_item_visible) {
        GameRenderer::DrawSpriteWorld(m_using_item);
    }

    GameRenderer::BeginOrderMode();
        GameRenderer::DrawAtlasSpriteWorld(m_left_hand.sprite);
        GameRenderer::DrawAtlasSpriteWorld(m_left_shoulder.sprite);
    GameRenderer::EndOrderMode();
}

void Player::use_item(const Camera& camera, World& world) {
    ZoneScopedN("Player::use_item");

    const ItemSlot taken_item = m_inventory.taken_item();
    const ItemSlot item_slot = taken_item.has_item() ? taken_item : m_inventory.get_selected_item();
    const std::optional<Item>& item = item_slot.item;
    if (!item) return;

    const glm::vec2& screen_pos = Input::MouseScreenPosition();
    
    // m_direction = screen_pos.x < camera.viewport().x * 0.5f ? Direction::Left : Direction::Right;

    if (m_swing_counter <= 0) {
        m_swing_counter = item->swing_speed;
        m_swing_counter_max = item->swing_speed;
    }

    m_using_item.set_texture(Assets::GetItemTexture(item->id));
    m_swing_anim = true;

    if (m_use_cooldown > 0) return;
    m_use_cooldown = item->use_cooldown;

    const glm::vec2 world_pos = camera.screen_to_world(screen_pos);

    const TilePos tile_pos = TilePos::from_world_pos(world_pos);

    bool used = false;

    if (item->is_pickaxe() && world.block_exists(tile_pos)) {
        Block* block = world.get_block_mut(tile_pos);

        if (block->hp > 0) {
            block->hp -= item->power;
        }

        const glm::vec2 position = tile_pos.to_world_pos_center();
        spawn_particles_on_dig(position, Particle::get_by_block(block->type), block->hp <= 0);
        
        if (block->hp <= 0) {
            world.remove_block(tile_pos);
            world.remove_block_cracks(tile_pos);
        } else {
            uint8_t new_variant = rand() % 3;
            BlockType new_block_type;
            switch (block->type) {
                case BlockType::Grass: new_block_type = BlockType::Dirt; break;
                default: new_block_type = block->type;
            }

            world.update_block(tile_pos, new_block_type, new_variant);

            world.create_dig_block_animation(*block, tile_pos);
            world.create_block_cracks(tile_pos, map_range(block_hp(block->type), 0, 0, 3, block->hp) * 6 + (rand() % 6));
        }

        used = true;
    } else if (item->is_hammer() && world.wall_exists(tile_pos)) {
        Wall* wall = world.get_wall_mut(tile_pos);

        if (wall->hp > 0) {
            wall->hp -= item->power;
        }

        const glm::vec2 position = tile_pos.to_world_pos_center();
        spawn_particles_on_dig(position, Particle::get_by_wall(wall->type), wall->hp <= 0);
        
        if (wall->hp <= 0) {
            world.remove_wall(tile_pos);
            world.remove_wall_cracks(tile_pos);
        } else {
            uint8_t new_variant = rand() % 3;

            world.update_wall(tile_pos, wall->type, new_variant);
            world.create_wall_cracks(tile_pos, map_range(wall_hp(wall->type), 0, 0, 3, wall->hp) * 6 + (rand() % 6));
        }

        used = true;
    } else if (item->places_tile.has_value()) {
        if (item->places_tile->type == TileType::Block && !world.block_exists(tile_pos)) {
            const math::Rect player_rect = math::Rect::from_center_half_size(m_position, glm::vec2(PLAYER_WIDTH_HALF, PLAYER_HEIGHT_HALF));
            const math::Rect tile_rect = math::Rect::from_center_size(tile_pos.to_world_pos_center(), glm::vec2(TILE_SIZE));

            if (!player_rect.intersects(tile_rect)) {
                world.set_block(tile_pos, item->places_tile->block);
                used = true;
            }
        } else if (item->places_tile->type == TileType::Wall && !world.wall_exists(tile_pos)) {
            world.set_wall(tile_pos, item->places_tile->wall);
            used = true;
        }
    }

    if (used) m_inventory.consume_item(item_slot.index);
}