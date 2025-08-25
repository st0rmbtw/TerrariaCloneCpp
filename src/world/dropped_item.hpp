#pragma once

#ifndef WORLD_DROPPED_ITEM_
#define WORLD_DROPPED_ITEM_

#include <SGE/types/sprite.hpp>
#include <glm/vec2.hpp>

#include "../types/item.hpp"
#include "../types/collision.hpp"
#include "../player/inventory.hpp"

#include "world_data.hpp"

class DroppedItem {
public:

    DroppedItem(const glm::vec2& position, const glm::vec2& velocity, const Item& item, bool set_timer = false) :
        m_position{ position },
        m_velocity{ velocity },
        m_item{ item },
        m_set_timer{ set_timer }
    {
        m_sprite.set_texture(Assets::GetItemTexture(item.id));
        m_sprite.set_position(position);
    }

    void draw() const;
    void update(const WorldData& world, float dt);

    bool follow_player(const sge::Rect& player_rect, const Inventory& inventory);

    void set_position(const glm::vec2& position) noexcept {
        m_position = position;
    }
    
    void set_velocity(const glm::vec2& velocity) noexcept {
        m_velocity = velocity;
    }

    void set_picked() noexcept {
        m_picked = true;
    }

    [[nodiscard]]
    const glm::vec2& position() const noexcept {
        return m_position;
    }

    [[nodiscard]]
    const glm::vec2& velocity() const noexcept {
        return m_velocity;
    }

    [[nodiscard]]
    glm::vec2 size() const noexcept {
        return m_sprite.size();
    }

    [[nodiscard]]
    const Item& item() const noexcept {
        return m_item;
    }

    [[nodiscard]]
    Item& item() noexcept {
        return m_item;
    }

    [[nodiscard]]
    bool picked() const noexcept {
        return m_picked;
    }

private:
    void apply_gravity();
    void apply_air_drag();
    void update_rotation();
    void keep_in_world_bounds(const WorldData& world);

    glm::vec2 check_collisions(const WorldData& world);

private:
    sge::Sprite m_sprite;
    glm::vec2 m_position = glm::vec2(0.0f);
    glm::vec2 m_velocity = glm::vec2(0.0f);
    Item m_item;
    Collision m_collision;

    float m_timer = 0.0f;

    bool m_set_timer = false;
    bool m_following = false;
    bool m_picked = false;
};

#endif