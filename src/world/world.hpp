#pragma once

#ifndef WORLD_WORLD_HPP_
#define WORLD_WORLD_HPP_

#include <cstdint>

#include <SGE/math/rect.hpp>
#include <SGE/renderer/camera.hpp>
#include <SGE/time/timer.hpp>
#include <SGE/time/time.hpp>

#include "../types/block.hpp"
#include "../types/wall.hpp"
#include "../types/tile_pos.hpp"
#include "../types/light.hpp"
#include "../types/item.hpp"
#include "../lookup_list.hpp"

#include "chunk_manager.hpp"
#include "dropped_item.hpp"

struct TileDigAnimation {
    TilePos tile_pos;
    TextureAtlasPos atlas_pos;
    float progress;
    float scale;
    BlockTypeWithData block;
};

struct TileCracks {
    TilePos tile_pos;
    uint8_t cracks_index;
};

class World {
public:
    void generate(uint32_t width, uint32_t height, uint32_t seed);

    void set_block(TilePos pos, const Block& block);
    void set_block(TilePos pos, BlockType block_type);
    void remove_block(TilePos pos);
    void update_block(TilePos pos, BlockTypeWithData new_block, uint8_t new_variant);

    void update_block_variant(TilePos pos, uint8_t new_variant);
    void update_block_type(TilePos pos, BlockType new_type);
    void update_block_data(TilePos pos, BlockData new_data);

    void set_wall(TilePos pos, WallType wall_type);
    void remove_wall(TilePos pos);
    void update_wall(TilePos pos, WallType new_type, uint8_t new_variant);

    void update_tile_sprite_index(TilePos pos);

    void update(const sge::Camera& camera);
    void fixed_update(const sge::Rect& player_rect, Inventory& inventory);

    void draw(const sge::Camera& camera) const;

    [[nodiscard]]
    inline std::optional<Block> get_block(TilePos pos) const {
        return m_data.get_block(pos);
    }

    [[nodiscard]]
    inline Block* get_block_mut(TilePos pos) {
        return m_data.get_block_mut(pos);
    }

    [[nodiscard]]
    inline std::optional<BlockType> get_block_type(TilePos pos) const {
        return m_data.get_block_type(pos);
    }

    [[nodiscard]]
    inline bool block_exists(TilePos pos) const {
        return m_data.block_exists(pos);
    }

    [[nodiscard]]
    inline bool block_exists_with_type(TilePos pos, BlockType block_type) const {
        return m_data.block_exists_with_type(pos, block_type);
    }

    [[nodiscard]]
    inline bool solid_block_exists(TilePos pos) const {
        return m_data.solid_block_exists(pos);
    }

    [[nodiscard]]
    inline Neighbors<Block> get_block_neighbors(TilePos pos) const {
        return m_data.get_block_neighbors(pos);
    }

    [[nodiscard]]
    inline Neighbors<BlockType> get_block_type_neighbors(TilePos pos) const {
        return m_data.get_block_type_neighbors(pos);
    }

    [[nodiscard]]
    inline Neighbors<Block*> get_block_neighbors_mut(TilePos pos) {
        return m_data.get_block_neighbors_mut(pos);
    }

    [[nodiscard]]
    inline std::optional<Wall> get_wall(TilePos pos) const {
        return m_data.get_wall(pos);
    }

    [[nodiscard]]
    inline Wall* get_wall_mut(TilePos pos) {
        return m_data.get_wall_mut(pos);
    }

    [[nodiscard]]
    inline bool wall_exists(TilePos pos) const {
        return m_data.wall_exists(pos);
    }

    [[nodiscard]]
    inline Neighbors<Wall> get_wall_neighbors(TilePos pos) const {
        return m_data.get_wall_neighbors(pos);
    }

    [[nodiscard]]
    inline Neighbors<Wall*> get_wall_neighbors_mut(TilePos pos) {
        return m_data.get_wall_neighbors_mut(pos);
    }

    inline glm::vec2 keep_in_world_bounds(glm::vec2 position, const glm::vec2& half_size) const noexcept {
        return m_data.keep_in_world_bounds(position, half_size);
    }

    [[nodiscard]]
    inline const sge::IRect& area() const noexcept {
        return m_data.area;
    }

    [[nodiscard]]
    inline const sge::IRect& playable_area() const noexcept {
        return m_data.playable_area;
    }

    [[nodiscard]]
    inline const glm::uvec2& spawn_point() const noexcept {
        return m_data.spawn_point;
    }

    [[nodiscard]]
    inline const Layers& layers() const noexcept {
        return m_data.layers;
    }

    [[nodiscard]]
    inline bool is_changed() const noexcept {
        return m_changed;
    }

    [[nodiscard]]
    inline bool is_lightmap_changed() const noexcept {
        return m_lightmap_changed;
    }

    [[nodiscard]]
    inline const ChunkManager& chunk_manager() const noexcept {
        return m_chunk_manager;
    }

    [[nodiscard]]
    inline ChunkManager& chunk_manager() noexcept {
        return m_chunk_manager;
    }

    [[nodiscard]]
    inline const WorldData& data() const noexcept {
        return m_data;
    }

    [[nodiscard]]
    inline WorldData& data() noexcept {
        return m_data;
    }

    inline void create_dig_tile_animation(const Block& block, TilePos pos) {
        m_tile_dig_animations.push_back(TileDigAnimation {
            .tile_pos = pos,
            .atlas_pos = block.atlas_pos,
            .progress = 0.0f,
            .scale = 0.0f,
            .block = block
        });
    }

    inline void create_block_cracks(TilePos pos, uint8_t cracks_index) {
        m_block_cracks[pos] = TileCracks {
            .tile_pos = pos,
            .cracks_index = cracks_index
        };
    }

    inline void create_wall_cracks(TilePos pos, uint8_t cracks_index) {
        m_wall_cracks[pos] = TileCracks {
            .tile_pos = pos,
            .cracks_index = cracks_index
        };
    }

    [[nodiscard]]
    inline const Light* lights() const noexcept {
        return m_lights;
    }

    [[nodiscard]]
    inline uint32_t light_count() const noexcept {
        return m_light_count;
    }

    inline void clear_lights() noexcept {
        m_light_count = 0;
    }

    inline void add_light(Light light) {
        using Constants::WORLD_MAX_LIGHT_COUNT;

        m_lights[m_light_count] = light;
        m_light_count = (m_light_count + 1) % WORLD_MAX_LIGHT_COUNT;
    }

    inline void drop_item(const glm::vec2& position, const glm::vec2& velocity, const Item& item, bool set_timer = false) {
        m_dropped_items.add(DroppedItem(position, velocity, item, set_timer));
    }

private:
    void update_neighbors(TilePos pos);
    void stack_dropped_items();

private:
    WorldData m_data;
    ChunkManager m_chunk_manager;
    LookupList<DroppedItem> m_dropped_items = LookupList<DroppedItem>(glm::vec2{ Constants::ITEM_GRAB_RANGE * 0.5f });
    std::unordered_set<size_t> m_pickedup_item_indices;
    std::unordered_map<TilePos, TileCracks> m_block_cracks;
    std::unordered_map<TilePos, TileCracks> m_wall_cracks;
    glm::vec2 m_offsets[7];
    sge::Timer m_anim_timer = sge::Timer::from_seconds(1.0f / 15.0f, sge::TimerMode::Repeating);
    std::vector<TileDigAnimation> m_tile_dig_animations;
    Light* m_lights = new Light[Constants::WORLD_MAX_LIGHT_COUNT];
    uint32_t m_light_count = 0;

    bool m_changed = false;
    bool m_lightmap_changed = false;
};

#endif