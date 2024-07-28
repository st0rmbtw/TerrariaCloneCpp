#include <glm/gtc/random.hpp>

#include "assets.hpp"
#include "player/player.hpp"
#include "common.h"
#include "constants.hpp"
#include "input.hpp"
#include "types/item.hpp"
#include "types/anchor.hpp"
#include "types/block.hpp"
#include "math/quat.hpp"
#include "renderer/renderer.hpp"
#include "math/math.hpp"
#include "math/rect.hpp"
#include "time/timer.hpp"
#include "utils.hpp"
#include "world/chunk.hpp"
#include "world/particles.hpp"

using Constants::TILE_SIZE;

void spawn_particles_on_dig(const glm::vec2& position, BlockType type) {
    const float rotation_speed = glm::pi<float>() / 12.0f;
    const Particle::Type particle = Particle::get_by_block(type);

    const int count = rand_range(3, 8);
    
    for (int i = 0; i < count; i++) {
        const glm::vec2 velocity = glm::normalize(glm::diskRand(1.0f)) * 1.25f;
        const float scale = glm::linearRand(0.3f, 1.0f);

        ParticleManager::spawn_particle(
            ParticleBuilder::create(particle, position, velocity, 0.75)
                .with_gravity(true)
                .with_rotation_speed(rotation_speed)
                .with_scale(scale)
        );
    }
}

void Player::init() {
    m_hair = TextureAtlasSprite(Assets::GetTextureAtlas(AssetKey::TexturePlayerHair));
    m_head = TextureAtlasSprite(Assets::GetTextureAtlas(AssetKey::TexturePlayerHead));
    m_body = TextureAtlasSprite(Assets::GetTextureAtlas(AssetKey::TexturePlayerChest));
    m_legs = TextureAtlasSprite(Assets::GetTextureAtlas(AssetKey::TexturePlayerLegs));
    m_left_hand = TextureAtlasSprite(Assets::GetTextureAtlas(AssetKey::TexturePlayerLeftHand));
    m_left_shoulder = TextureAtlasSprite(Assets::GetTextureAtlas(AssetKey::TexturePlayerLeftShoulder));
    m_right_arm = TextureAtlasSprite(Assets::GetTextureAtlas(AssetKey::TexturePlayerRightArm));
    m_left_eye = TextureAtlasSprite(Assets::GetTextureAtlas(AssetKey::TexturePlayerLeftEye));
    m_right_eye = TextureAtlasSprite(Assets::GetTextureAtlas(AssetKey::TexturePlayerRightEye));
    m_walk_anim_timer = Timer(Timer::Duration::zero(), TimerMode::Repeating);
    m_walk_particles_timer = Timer(duration::seconds_float(1.0f / 20.0f), TimerMode::Repeating);
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

    m_hair.sprite.set_color(glm::vec4(0.55, 0.23, 0.14, 1.0));
    m_head.sprite.set_color(glm::vec4(0.92, 0.45, 0.32, 1.0));
    m_body.sprite.set_color(glm::vec4(0.58, 0.55, 0.47, 1.0));
    m_legs.sprite.set_color(glm::vec4(190.0 / 255.0, 190.0 / 255.0, 156.0 / 255.0, 1.0));

    m_left_hand.sprite.set_color(glm::vec4(0.92, 0.45, 0.32, 1.0));
    m_left_shoulder.sprite.set_color(glm::vec4(0.58, 0.55, 0.47, 1.0));
    m_right_arm.sprite.set_color(glm::vec4(0.92, 0.45, 0.32, 1.0));
    m_right_eye.sprite.set_color(glm::vec4(89.0 / 255.0, 76.0 / 255.0, 64.0 / 255.0, 1.0));

    m_legs.sprite.set_color(glm::vec4(190.0 / 255.0, 190.0 / 255.0, 156.0 / 255.0, 1.0));
}

void Player::set_position(const World& world, const glm::vec2& position) {
    m_position.x = position.x;
    m_position.y = position.y - PLAYER_HEIGHT_HALF;
    keep_in_world_bounds(world);
}

void Player::horizontal_movement(bool handle_input) {
    int8_t dir = 0;

    if (handle_input && KeyboardInput::Pressed(Key::A)) { dir -= 1; }
    if (handle_input && KeyboardInput::Pressed(Key::D)) { dir += 1; }

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
    if (do_jump && m_collisions.down) {
        m_jump = JUMP_HEIGHT;
        m_velocity.y = -JUMP_SPEED;
        m_jumping = true;
    }

    if (handle_input && KeyboardInput::Pressed(Key::Space)) {
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

void Player::gravity(void) {
    if (!m_collisions.down && m_velocity.y > 0 && m_fall_start < 0) {
        m_fall_start = m_position.y;
    }

    m_velocity.y += GRAVITY;
    m_velocity.y = glm::clamp(m_velocity.y, -MAX_FALL_SPEED, MAX_FALL_SPEED);
}

glm::vec2 Player::check_collisions(const World& world) {
    glm::vec2 result = m_velocity;
    glm::vec2 pos = m_position;
    glm::vec2 next_pos = m_position + m_velocity;

    int left = static_cast<int>((m_position.x - PLAYER_WIDTH_HALF) / TILE_SIZE) - 1;
    int right = static_cast<int>((m_position.x + PLAYER_WIDTH_HALF) / TILE_SIZE) + 2;
    int top = static_cast<int>((m_position.y - PLAYER_HEIGHT_HALF) / TILE_SIZE) - 1;
    int bottom = static_cast<int>((m_position.y + PLAYER_HEIGHT_HALF) / TILE_SIZE) + 2;

    left = glm::clamp(left, static_cast<int>(world.playable_area().min.x), static_cast<int>(world.playable_area().max.x));
    right = glm::clamp(right, static_cast<int>(world.playable_area().min.x), static_cast<int>(world.playable_area().max.x));
    top = glm::clamp(top, static_cast<int>(world.playable_area().min.y), static_cast<int>(world.playable_area().max.y));
    bottom = glm::clamp(bottom, static_cast<int>(world.playable_area().min.y), static_cast<int>(world.playable_area().max.y));

    int num5 = -1;
    int num6 = -1;
    int num7 = -1;
    int num8 = -1;

    m_collisions = Collisions();

    for (uint32_t y = top; y < bottom; ++y) {
        for (uint32_t x = left; x < right; ++x) {
            const glm::vec2 tile_pos = glm::vec2(x * TILE_SIZE, y * TILE_SIZE);
            if (world.block_exists(TilePos(x, y))) {
                
                if (next_pos.x + PLAYER_WIDTH_HALF > tile_pos.x && next_pos.x - PLAYER_WIDTH_HALF < tile_pos.x + TILE_SIZE &&
                    next_pos.y + PLAYER_HEIGHT_HALF > tile_pos.y && next_pos.y - PLAYER_HEIGHT_HALF < tile_pos.y + TILE_SIZE)
                {
                    if (pos.y + PLAYER_HEIGHT_HALF <= tile_pos.y) {
                        m_collisions.down = true;
                        m_jumping = false;
                        m_stand_on_block = world.get_block_type(TilePos(x, y));
                        num7 = x;
                        num8 = y;
                        if (num7 != num5) {
                            result.y = tile_pos.y - (pos.y + PLAYER_HEIGHT_HALF);
                        }
                    } else if (pos.x + PLAYER_WIDTH_HALF <= tile_pos.x) {
                        m_collisions.right = true;
                        num5 = x;
                        num6 = y;
                        if (num6 != num8) {
                            result.x = tile_pos.x - (pos.x + PLAYER_WIDTH_HALF);
                        }
                    } else if (pos.x + PLAYER_WIDTH_HALF >= tile_pos.x + TILE_SIZE) {
                        m_collisions.left = true;
                        num5 = x;
                        num6 = y;
                        if (num6 != num8) {
                            result.x = tile_pos.x + TILE_SIZE - (pos.x - PLAYER_WIDTH_HALF);
                        }
                    } else if (pos.y - PLAYER_HEIGHT_HALF >= tile_pos.y + TILE_SIZE) {
                        m_collisions.up = true;
                        num7 = x;
                        num8 = y;
                        result.y = tile_pos.y + TILE_SIZE - (pos.y - PLAYER_HEIGHT_HALF);
                        if (num8 == num6) {
                            result.x = m_velocity.x;
                        }
                    }
                }
            }
        }
    }

    return result;
}

void Player::update_walk_anim_timer(void) {
    if (m_velocity.x != 0.0) {
        const long long time = glm::abs(100.0 / glm::abs(m_velocity.x));
        m_walk_anim_timer.set_duration(Timer::Duration(glm::max(time, 1LL)));
    }
}

void Player::update_using_item_anim(void) {
    m_using_item_visible = false;
    
    if (!m_swing_anim) return;

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

    const glm::vec2 item_position = m_position + offset;

    // 0..1
    float rotation = static_cast<float>(m_swing_counter) / static_cast<float>(m_swing_counter_max);
    // -1..1
    rotation = rotation * 2.0 - 1.;

    m_using_item.set_position(item_position);
    m_using_item.set_rotation(Quat::from_rotation_z(rotation * direction * ITEM_ROTATION + direction * 0.5f));
    m_using_item.set_anchor(m_direction == Direction::Left ? Anchor::BottomRight : Anchor::BottomLeft);
    m_using_item.set_flip_x(m_direction == Direction::Left ? true : false);
    
    m_using_item_visible = true;
}

void Player::update_sprites_index(const delta_time_t& delta_time) {
    PlayerSprite* sprites[] = { &m_hair, &m_head, &m_body, &m_legs, &m_left_hand, &m_left_shoulder, &m_right_arm, &m_left_eye, &m_right_eye };

    switch (m_movement_state) {
    case MovementState::Walking: {
        if (m_walk_anim_timer.tick(delta_time).just_finished()) {
            m_walk_animation_index = (m_walk_animation_index + 1) % WALK_ANIMATION_LENGTH;

            for (PlayerSprite* sprite : sprites) {
                const int index = sprite->walk_animation.offset + map_range(
                    0, WALK_ANIMATION_LENGTH,
                    0, sprite->walk_animation.length,
                    m_walk_animation_index
                );
                sprite->sprite.set_index(index);
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
    }
}

void Player::update_movement_state(void) {
    if (m_velocity.y != 0) {
        m_movement_state = MovementState::Flying;
    } else if (m_velocity.x != 0) {
        m_movement_state = MovementState::Walking;
    } else {
        m_movement_state = MovementState::Idle;
    }
}

void Player::pre_update() {
    if (KeyboardInput::JustPressed(Key::Space)) {
        do_jump = true;
    }
}

void Player::spawn_particles_on_walk(void) const {
    if (m_stand_on_block.is_none()) return;
    BlockType block = m_stand_on_block.value();

    if (!block_dusty(block)) return;
    if (m_movement_state != MovementState::Walking) return;
    if (glm::abs(m_velocity.x) < 1.0f) return;

    const float direction = m_direction == Direction::Right ? -1.0f : 1.0f;

    const glm::vec2 position = m_position + glm::vec2(0., PLAYER_HEIGHT_HALF);
    const glm::vec2 velocity = random_point_cone(glm::vec2(direction, 0.0f), 45.0f);
    const float scale = rand_range(0.0f, 1.0f);
    const float rotation_speed = glm::pi<float>() / 12.0f;

    ParticleManager::spawn_particle(
        ParticleBuilder::create(Particle::get_by_block(block), position, velocity, 0.3f)
            .with_scale(scale)
            .with_rotation_speed(rotation_speed)
    );
}

void Player::spawn_particles_grounded(void) const {
    if (m_stand_on_block.is_none()) return;
    BlockType block = m_stand_on_block.value();

    if (!block_dusty(block)) return;

    const Particle::Type particle = Particle::get_by_block(block);

    const float fall_distance = get_fall_distance();
    const float rotation_speed = glm::pi<float>() / 12.0f;
    const glm::vec2 position = m_position + glm::vec2(0., PLAYER_HEIGHT_HALF);

    if (!m_prev_grounded && m_collisions.down && fall_distance > TILE_SIZE * 1.5) {
        for (int i = 0; i < 10; i++) {
            const float scale = rand_range(0.0f, 1.0f);
            const glm::vec2 point = random_point_circle(1.0f, 0.5f) * PLAYER_WIDTH_HALF;
            const glm::vec2 velocity = glm::vec2(glm::normalize(point).x, -0.5f);

            ParticleManager::spawn_particle(
                ParticleBuilder::create(particle, position, velocity, 0.3f)
                    .with_scale(scale)
                    .with_rotation_speed(rotation_speed)
            );
        }
    }
}

void Player::fixed_update(const delta_time_t& delta_time, const World& world, bool handle_input) {
    horizontal_movement(handle_input);
    vertical_movement(handle_input);
    gravity();

    m_velocity = check_collisions(world);
    m_position += m_velocity;

    keep_in_world_bounds(world);

    update_movement_state();
    update_walk_anim_timer();

    update_using_item_anim();

    update_sprites_index(delta_time);

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
    if (MouseInput::Pressed(MouseButton::Left) && !MouseInput::IsMouseOverUi()) {
        use_item(camera, world);
    }

    update_sprites();
}

void Player::keep_in_world_bounds(const World& world) {
    const float world_min_x = world.playable_area().min.x * TILE_SIZE;
    const float world_max_x = world.playable_area().max.x * TILE_SIZE;
    const float world_min_y = world.playable_area().min.y * TILE_SIZE;
    const float world_max_y = world.playable_area().max.y * TILE_SIZE;

    if (m_position.x - PLAYER_WIDTH_HALF < world_min_x) m_position.x = world_min_x + PLAYER_WIDTH_HALF;
    if (m_position.x + PLAYER_WIDTH_HALF > world_max_x) m_position.x = world_max_x - PLAYER_WIDTH_HALF;
    if (m_position.y - PLAYER_HEIGHT_HALF < world_min_y) m_position.y = world_min_y + PLAYER_HEIGHT_HALF;
    if (m_position.y + PLAYER_HEIGHT_HALF > world_max_y) m_position.y = world_max_y - PLAYER_HEIGHT_HALF;
}

float Player::get_fall_distance(void) const {
    if (m_fall_start < 0) return 0;
    return m_position.y + PLAYER_HEIGHT_HALF - m_fall_start;
}

void Player::update_sprites(void) {
    m_hair.sprite
        .set_flip_x(m_direction == Direction::Left)
        .set_position(m_position);
    m_head.sprite
        .set_flip_x(m_direction == Direction::Left)
        .set_position(m_position);
    m_body.sprite
        .set_flip_x(m_direction == Direction::Left)
        .set_position(m_position);
    m_legs.sprite
        .set_flip_x(m_direction == Direction::Left)
        .set_position(m_position);
    m_left_hand.sprite
        .set_flip_x(m_direction == Direction::Left)
        .set_position(m_position);
    m_left_shoulder.sprite
        .set_flip_x(m_direction == Direction::Left)
        .set_position(m_position);
    m_right_arm.sprite
        .set_flip_x(m_direction == Direction::Left)
        .set_position(m_position);
    m_left_eye.sprite
        .set_flip_x(m_direction == Direction::Left)
        .set_position(m_position);
    m_right_eye.sprite
        .set_flip_x(m_direction == Direction::Left)
        .set_position(m_position);
}

void Player::render(void) const {
    Renderer::DrawAtlasSprite(m_head.sprite, RenderLayer::World);

    Renderer::DrawAtlasSprite(m_right_arm.sprite, RenderLayer::World);

    Renderer::DrawAtlasSprite(m_hair.sprite, RenderLayer::World);
    Renderer::DrawAtlasSprite(m_body.sprite, RenderLayer::World);
    Renderer::DrawAtlasSprite(m_legs.sprite, RenderLayer::World);

    Renderer::DrawAtlasSprite(m_left_eye.sprite, RenderLayer::World);
    Renderer::DrawAtlasSprite(m_right_eye.sprite, RenderLayer::World);

    if (m_using_item_visible) {
        Renderer::DrawSprite(m_using_item, RenderLayer::World);
    }

    Renderer::DrawAtlasSprite(m_left_hand.sprite, RenderLayer::World);
    Renderer::DrawAtlasSprite(m_left_shoulder.sprite, RenderLayer::World);
}

void Player::use_item(const Camera& camera, World& world) {
    const tl::optional<const Item&> item = m_inventory.get_selected_item();
    if (item.is_none()) return;

    if (m_swing_counter <= 0) {
        m_swing_counter = item->swing_speed;
        m_swing_counter_max = item->swing_speed;
    }

    m_using_item.set_texture(Assets::GetItemTexture(item->id));
    m_swing_anim = true;

    if (m_use_cooldown > 0) return;

    const glm::vec2& screen_pos = MouseInput::ScreenPosition();
    const glm::vec2 world_pos = camera.screen_to_world(screen_pos);

    const TilePos tile_pos = TilePos::from_world_pos(world_pos);

    if (item->is_pickaxe && world.block_exists(tile_pos)) {
        tl::optional<Block&> block = world.get_block_mut(tile_pos);

        if (block->hp > 0) {
            block->hp -= item->power;
        }

        const glm::vec2 position = tile_pos.to_world_pos_center();
        spawn_particles_on_dig(position, block->type);

        if (block->type == BlockType::Grass) {
            world.update_block_type(tile_pos, BlockType::Dirt);
        }
        
        if (block->hp <= 0) {
            world.remove_block(tile_pos);
        }
    } else if (item->places_block.is_some() && !world.block_exists(tile_pos)) {
        world.set_block(tile_pos, item->places_block.value());
    }

    m_use_cooldown = item->use_cooldown;
}