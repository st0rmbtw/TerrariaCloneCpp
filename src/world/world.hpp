#ifndef WORLD_WORLD_HPP
#define WORLD_WORLD_HPP

#pragma once

#include <cstdint>

#include <SGE/math/rect.hpp>
#include <SGE/renderer/camera.hpp>
#include <SGE/time/timer.hpp>

#include "../types/block.hpp"
#include "../types/wall.hpp"
#include "../types/tile_pos.hpp"
#include "../types/light.hpp"

#include "chunk_manager.hpp"

struct TileDigAnimation {
    TilePos tile_pos;
    TextureAtlasPos atlas_pos;
    float progress;
    float scale;
    TileType tile_type;
};

struct TileCracks {
    TilePos tile_pos;
    uint8_t cracks_index;
};

class World {
public:
    World() = default;

    void generate(uint32_t width, uint32_t height, uint32_t seed);

    void set_tile(TilePos pos, const Tile& block);
    void set_tile(TilePos pos, TileType block_type);
    void remove_tile(TilePos pos);
    void update_tile(TilePos pos, TileType new_type, uint8_t new_variant);

    void set_wall(TilePos pos, WallType wall_type);
    void remove_wall(TilePos pos);
    void update_wall(TilePos pos, WallType new_type, uint8_t new_variant);

    void update_tile_sprite_index(TilePos pos);

    void update(const sge::Camera& camera);
    
    void draw() const;

    [[nodiscard]] inline std::optional<Tile> get_tile(TilePos pos) const { return m_data.get_tile(pos); }
    [[nodiscard]] inline Tile* get_tile_mut(TilePos pos) { return m_data.get_tile_mut(pos); }

    [[nodiscard]] inline std::optional<TileType> get_tile_type(TilePos pos) const { return m_data.get_tile_type(pos); }

    [[nodiscard]] inline bool tile_exists(TilePos pos) const { return m_data.tile_exists(pos); }
    [[nodiscard]] inline bool tile_exists_with_type(TilePos pos, TileType block_type) const { return m_data.tile_exists_with_type(pos, block_type); }
    [[nodiscard]] inline bool solid_tile_exists(TilePos pos) const { return m_data.solid_tile_exists(pos); }

    [[nodiscard]] inline Neighbors<Tile> get_tile_neighbors(TilePos pos) const { return m_data.get_tile_neighbors(pos); }
    [[nodiscard]] inline Neighbors<TileType> get_tile_type_neighbors(TilePos pos) const { return m_data.get_tile_type_neighbors(pos); }
    [[nodiscard]] inline Neighbors<Tile*> get_tile_neighbors_mut(TilePos pos) { return m_data.get_tile_neighbors_mut(pos); }

    [[nodiscard]] inline std::optional<Wall> get_wall(TilePos pos) const { return m_data.get_wall(pos); }
    [[nodiscard]] inline Wall* get_wall_mut(TilePos pos) { return m_data.get_wall_mut(pos); }

    [[nodiscard]] inline bool wall_exists(TilePos pos) const { return m_data.wall_exists(pos); }

    [[nodiscard]] inline Neighbors<Wall> get_wall_neighbors(TilePos pos) const { return m_data.get_wall_neighbors(pos); }
    [[nodiscard]] inline Neighbors<Wall*> get_wall_neighbors_mut(TilePos pos) { return m_data.get_wall_neighbors_mut(pos); }

    [[nodiscard]] inline const sge::IRect& area() const { return m_data.area; }
    [[nodiscard]] inline const sge::IRect& playable_area() const { return m_data.playable_area; }
    [[nodiscard]] inline const glm::uvec2& spawn_point() const { return m_data.spawn_point; }
    [[nodiscard]] inline const Layers& layers() const { return m_data.layers; }

    [[nodiscard]] inline bool is_changed() const { return m_changed; }
    [[nodiscard]] inline bool is_lightmap_changed() const { return m_lightmap_changed; }

    [[nodiscard]] inline const ChunkManager& chunk_manager() const { return m_chunk_manager; }
    [[nodiscard]] inline ChunkManager& chunk_manager() { return m_chunk_manager; }

    [[nodiscard]] inline const WorldData& data() const { return m_data; }
    [[nodiscard]] inline WorldData& data() { return m_data; }

    [[nodiscard]] inline void create_dig_tile_animation(const Tile& tile, TilePos pos) {
        m_tile_dig_animations.push_back(TileDigAnimation {
            .tile_pos = pos,
            .atlas_pos = tile.atlas_pos,
            .progress = 0.0f,
            .scale = 0.0f,
            .tile_type = tile.type
        });
    }

    [[nodiscard]] inline void create_tile_cracks(TilePos pos, uint8_t cracks_index) {
        m_tile_cracks[pos] = TileCracks {
            .tile_pos = pos,
            .cracks_index = cracks_index
        };
    }

    inline void remove_tile_cracks(TilePos pos) {
        m_tile_cracks.erase(pos);
    }

    [[nodiscard]] inline void create_wall_cracks(TilePos pos, uint8_t cracks_index) {
        m_wall_cracks[pos] = TileCracks {
            .tile_pos = pos,
            .cracks_index = cracks_index
        };
    }

    inline void remove_wall_cracks(TilePos pos) {
        m_wall_cracks.erase(pos);
    }

    [[nodiscard]] inline const Light* lights() const { return m_lights; }
    [[nodiscard]] inline uint32_t light_count() const { return m_light_count; }

    inline void clear_lights() { m_light_count = 0; }

    [[nodiscard]] inline void add_light(Light light) {
        using Constants::WORLD_MAX_LIGHT_COUNT;

        m_lights[m_light_count] = light;
        m_light_count = (m_light_count + 1) % WORLD_MAX_LIGHT_COUNT;
    }

private:
    void update_neighbors(TilePos pos);

private:
    WorldData m_data;
    ChunkManager m_chunk_manager;
    std::vector<TileDigAnimation> m_tile_dig_animations;
    std::unordered_map<TilePos, TileCracks> m_tile_cracks;
    std::unordered_map<TilePos, TileCracks> m_wall_cracks;
    sge::Timer m_anim_timer = sge::Timer::from_seconds(1.0f / 15.0f, sge::TimerMode::Repeating);
    Light* m_lights = nullptr;
    uint32_t m_light_count = 0; 

    glm::vec2 offsets[7];

    bool m_changed = false;
    bool m_lightmap_changed = false;
};

#endif