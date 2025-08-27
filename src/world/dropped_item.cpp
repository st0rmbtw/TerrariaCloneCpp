#include <SGE/profile.hpp>
#include <SGE/math/rect.hpp>
#include <SGE/time/time.hpp>

#include "../renderer/renderer.hpp"

#include "dropped_item.hpp"

static constexpr float GRAVITY = 0.1f;
static constexpr float MAX_VERTICAL_SPEED = 7.f;
static constexpr float MAX_HORIZONTAL_SPEED = 5.f;
static constexpr float PLAYER_GRAB_DELAY = 1.5f;

using Constants::ITEM_GRAB_RANGE;

void DroppedItem::draw() const {
    GameRenderer::DrawSpriteWorld(m_sprite);
}

void DroppedItem::update(const WorldData& world, float dt) {
    ZoneScoped;

    if (m_set_timer && m_timer < PLAYER_GRAB_DELAY)
        m_timer += dt;

    apply_gravity();
    apply_air_drag();

    m_velocity = check_collisions(world);
    m_position += m_velocity;

    keep_in_world_bounds(world);

    update_rotation();
    m_sprite.set_position(m_position);
}

void DroppedItem::apply_gravity() {
    m_velocity.y += GRAVITY;
    m_velocity.y = glm::clamp(m_velocity.y, -MAX_VERTICAL_SPEED, MAX_VERTICAL_SPEED);
}

void DroppedItem::apply_air_drag() {
    m_velocity.x *= 0.95f;

    if (m_velocity.x < 0.1f && m_velocity.x > -0.1f) {
        m_velocity.x = 0.;
    }

    m_velocity.x = glm::clamp(m_velocity.x, -MAX_HORIZONTAL_SPEED, MAX_HORIZONTAL_SPEED);
}

void DroppedItem::update_rotation() {
    const glm::vec2 direction = glm::normalize(glm::vec2(m_velocity.x / MAX_HORIZONTAL_SPEED, 1.0f));
    const float angle = glm::atan(direction.x, direction.y);
    m_sprite.set_rotation(glm::angleAxis(angle, glm::vec3(0.0f, 0.0f, 1.0f)));
}

void DroppedItem::keep_in_world_bounds(const WorldData& world) {
    const glm::vec2 new_pos = world.keep_in_world_bounds(m_position, m_sprite.size() * 0.5f);

    if (m_position.x != new_pos.x) {
        m_velocity.x = 0.0f;
    }

    if (m_position.y != new_pos.y) {
        m_velocity.y = 0.0f;
    }
}

glm::vec2 DroppedItem::check_collisions(const WorldData& world) {
    ZoneScoped;

    using Constants::TILE_SIZE;

    if (m_following)
        return m_velocity;

    glm::vec2 result = m_velocity;
    const glm::vec2 pos = m_position;
    const glm::vec2 next_pos = m_position + m_velocity;
    const sge::IRect& area = world.playable_area;

    const glm::vec2 size_half = m_sprite.size() * 0.5f;

    int left = static_cast<int>((m_position.x - size_half.x) / TILE_SIZE) - 1;
    int right = static_cast<int>((m_position.x + size_half.x) / TILE_SIZE) + 2;
    int top = static_cast<int>((m_position.y - size_half.y) / TILE_SIZE) - 1;
    int bottom = static_cast<int>((m_position.y + size_half.y) / TILE_SIZE) + 2;

    left = glm::clamp(left, area.min.x, area.max.x);
    right = glm::clamp(right, area.min.x, area.max.x);
    top = glm::clamp(top, area.min.y, area.max.y);
    bottom = glm::clamp(bottom, area.min.y, area.max.y);

    int hx = -1;
    int hy = -1;
    int vx = -1;
    int vy = -1;

    m_collision.reset();

    for (int y = top; y < bottom; ++y) {
        for (int x = left; x < right; ++x) {
            if (!world.solid_block_exists(TilePos(x, y))) continue;

            const glm::vec2 tile_pos = glm::vec2(x * TILE_SIZE, y * TILE_SIZE);

            if (
                next_pos.x + size_half.x > tile_pos.x && next_pos.x - size_half.x < tile_pos.x + TILE_SIZE &&
                next_pos.y + size_half.y > tile_pos.y && next_pos.y - size_half.y < tile_pos.y + TILE_SIZE
            ) {
                if (pos.y + size_half.y <= tile_pos.y) {
                    vx = x;
                    vy = y;
                    if (vx != hx) {
                        m_collision.set_down(true);

                        result.y = tile_pos.y - (pos.y + size_half.y);
                    }
                } else if (pos.x + size_half.x <= tile_pos.x) {
                    hx = x;
                    hy = y;
                    if (hy != vy) {
                        result.x = tile_pos.x - (pos.x + size_half.x);
                        m_collision.set_right(true);
                    }
                    if (vx == hx) {
                        result.y = m_velocity.y;
                    }
                } else if (pos.x - size_half.x >= tile_pos.x + TILE_SIZE) {
                    m_collision.set_left(true);
                    hx = x;
                    hy = y;
                    if (hy != vy) {
                        result.x = tile_pos.x + TILE_SIZE - (pos.x - size_half.x);
                    }
                    if (vx == hx) {
                        result.y = m_velocity.y;
                    }
                } else if (pos.y - size_half.y >= tile_pos.y + TILE_SIZE) {
                    m_collision.set_up(true);

                    vx = x;
                    vy = y;
                    result.y = tile_pos.y + TILE_SIZE - (pos.y - size_half.y);
                    if (vy == hy) {
                        result.x = m_velocity.x;
                    }
                }
            }
        }
    }

    return result;
}

bool DroppedItem::follow_player(const sge::Rect& player_rect, const Inventory& inventory) {
    const sge::Rect grab_rect = sge::Rect::from_center_size(player_rect.center(), glm::vec2{ ITEM_GRAB_RANGE });
    const sge::Rect item_rect = sge::Rect::from_center_size(m_position, m_sprite.size());

    if (!grab_rect.intersects(item_rect) || !inventory.can_be_added(m_item)) {
        m_following = false;
        return false;
    }

    if (m_timer && m_timer < PLAYER_GRAB_DELAY) {
        return false;
    }

    const float distance = glm::length(player_rect.center() - item_rect.center());

    if (distance <= 16.0f)
        return true;

    const glm::vec2 direction = glm::normalize(player_rect.center() - item_rect.center());

    m_velocity = direction * 4.0f;
    m_following = true;

    return false;
}