#ifndef WORLD_WORLD_HPP
#define WORLD_WORLD_HPP

#pragma once

#include <cstdint>

#include "../engine/math/rect.hpp"
#include "../engine/renderer/camera.hpp"

#include "../types/block.hpp"
#include "../types/wall.hpp"
#include "../types/tile_pos.hpp"
#include "../types/light.hpp"

#include "chunk_manager.hpp"

struct BlockDigAnimation {
    TilePos tile_pos;
    TextureAtlasPos atlas_pos;
    float progress;
    float scale;
    BlockType block_type;
};

struct TileCracks {
    TilePos tile_pos;
    uint8_t cracks_index;
};

static constexpr size_t WORLD_MAX_LIGHT_COUNT = 0xFFFF / sizeof(Light);

class World {
public:
    World() = default;

    void generate(uint32_t width, uint32_t height, uint32_t seed);

    void set_block(TilePos pos, const Block& block);
    void set_block(TilePos pos, BlockType block_type);
    void remove_block(TilePos pos);
    void update_block(TilePos pos, BlockType new_type, uint8_t new_variant);

    void set_wall(TilePos pos, WallType wall_type);
    void remove_wall(TilePos pos);
    void update_wall(TilePos pos, WallType new_type, uint8_t new_variant);

    void update_tile_sprite_index(TilePos pos);

    void update(const Camera& camera);
    
    void draw() const;

    [[nodiscard]] inline std::optional<Block> get_block(TilePos pos) const { return m_data.get_block(pos); }
    [[nodiscard]] inline Block* get_block_mut(TilePos pos) { return m_data.get_block_mut(pos); }

    [[nodiscard]] inline std::optional<BlockType> get_block_type(TilePos pos) const { return m_data.get_block_type(pos); }

    [[nodiscard]] inline bool block_exists(TilePos pos) const { return m_data.block_exists(pos); }
    [[nodiscard]] inline bool block_exists_with_type(TilePos pos, BlockType block_type) const { return m_data.block_exists_with_type(pos, block_type); }

    [[nodiscard]] inline Neighbors<Block> get_block_neighbors(TilePos pos) const { return m_data.get_block_neighbors(pos); }
    [[nodiscard]] inline Neighbors<Block*> get_block_neighbors_mut(TilePos pos) { return m_data.get_block_neighbors_mut(pos); }

    [[nodiscard]] inline std::optional<Wall> get_wall(TilePos pos) const { return m_data.get_wall(pos); }
    [[nodiscard]] inline Wall* get_wall_mut(TilePos pos) { return m_data.get_wall_mut(pos); }

    [[nodiscard]] inline bool wall_exists(TilePos pos) const { return m_data.wall_exists(pos); }

    [[nodiscard]] inline Neighbors<Wall> get_wall_neighbors(TilePos pos) const { return m_data.get_wall_neighbors(pos); }
    [[nodiscard]] inline Neighbors<Wall*> get_wall_neighbors_mut(TilePos pos) { return m_data.get_wall_neighbors_mut(pos); }

    [[nodiscard]] inline const math::IRect& area() const { return m_data.area; }
    [[nodiscard]] inline const math::IRect& playable_area() const { return m_data.playable_area; }
    [[nodiscard]] inline const glm::uvec2& spawn_point() const { return m_data.spawn_point; }
    [[nodiscard]] inline const Layers& layers() const { return m_data.layers; }

    [[nodiscard]] inline bool is_changed() const { return m_changed; }
    [[nodiscard]] inline bool is_lightmap_changed() const { return m_lightmap_changed; }

    [[nodiscard]] inline const ChunkManager& chunk_manager() const { return m_chunk_manager; }
    [[nodiscard]] inline ChunkManager& chunk_manager() { return m_chunk_manager; }

    [[nodiscard]] inline const WorldData& data() const { return m_data; }
    [[nodiscard]] inline WorldData& data() { return m_data; }

    [[nodiscard]] inline void create_dig_block_animation(const Block& block, TilePos pos) {
        m_block_dig_animations.push_back(BlockDigAnimation {
            .tile_pos = pos,
            .atlas_pos = block.atlas_pos,
            .progress = 0.0f,
            .scale = 0.0f,
            .block_type = block.type
        });
    }

    [[nodiscard]] inline void create_block_cracks(TilePos pos, uint8_t cracks_index) {
        m_block_cracks[pos] = TileCracks {
            .tile_pos = pos,
            .cracks_index = cracks_index
        };
    }

    inline void remove_block_cracks(TilePos pos) {
        m_block_cracks.erase(pos);
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
        m_lights[m_light_count] = light;
        m_light_count = (m_light_count + 1) % WORLD_MAX_LIGHT_COUNT;
    }

private:
    void update_neighbors(TilePos pos);

private:
    WorldData m_data;
    ChunkManager m_chunk_manager;
    std::vector<BlockDigAnimation> m_block_dig_animations;
    std::unordered_map<TilePos, TileCracks> m_block_cracks;
    std::unordered_map<TilePos, TileCracks> m_wall_cracks;
    Light* m_lights = nullptr;
    uint32_t m_light_count = 0; 
    bool m_changed = false;
    bool m_lightmap_changed = false;
};

#endif