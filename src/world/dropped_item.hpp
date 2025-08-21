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

    DroppedItem(const glm::vec2& position, const glm::vec2& velocity, const Item& item, std::optional<float> spawn_time = std::nullopt) :
        m_position{ position },
        m_velocity{ velocity },
        m_item{ item },
        m_spawn_time{ spawn_time }
    {
        m_sprite.set_texture(Assets::GetItemTexture(item.id));
        m_sprite.set_position(position);
    }

    void draw() const;
    void update(const WorldData& world);

    bool follow_player(const sge::Rect& player_rect, const Inventory& inventory);

    [[nodiscard]]
    const glm::vec2& position() const noexcept {
        return m_position;
    }

    [[nodiscard]]
    const Item& item() const noexcept {
        return m_item;
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

    std::optional<float> m_spawn_time;

    bool m_following = false;
};

#endif