#ifndef WORLD_WORLD_HPP
#define WORLD_WORLD_HPP

#pragma once

#include <cstdint>

#include "../math/rect.hpp"
#include "../renderer/camera.h"

#include "../types/block.hpp"
#include "../types/wall.hpp"
#include "../types/tile_pos.hpp"
#include "../optional.hpp"

#include "chunk_manager.hpp"

struct BlockDigAnimation {
    TilePos tile_pos;
    TextureAtlasPos atlas_pos;
    float progress;
    float scale;
    BlockType block_type;
};

class World {
public:
    World() = default;

    void generate(uint32_t width, uint32_t height, uint32_t seed);

    void set_block(TilePos pos, const Block& block);
    void set_block(TilePos pos, BlockType block_type);
    void remove_block(TilePos pos);

    void update_block(TilePos pos, BlockType new_type, uint8_t new_variant);

    void set_wall(TilePos pos, WallType wall_type);

    void update_tile_sprite_index(TilePos pos);

    void update(const Camera& camera);
    
    void draw() const;

    [[nodiscard]] inline tl::optional<const Block&> get_block(TilePos pos) const { return m_data.get_block(pos); }
    [[nodiscard]] inline tl::optional<Block&> get_block_mut(TilePos pos) { return m_data.get_block_mut(pos); }

    [[nodiscard]] inline tl::optional<BlockType> get_block_type(TilePos pos) const { return m_data.get_block_type(pos); }

    [[nodiscard]] inline bool block_exists(TilePos pos) const { return m_data.block_exists(pos); }
    [[nodiscard]] inline bool block_exists_with_type(TilePos pos, BlockType block_type) const { return m_data.block_exists_with_type(pos, block_type); }

    [[nodiscard]] inline Neighbors<const Block&> get_block_neighbors(TilePos pos) const { return m_data.get_block_neighbors(pos); }
    [[nodiscard]] inline Neighbors<Block&> get_block_neighbors_mut(TilePos pos) { return m_data.get_block_neighbors_mut(pos); }

    [[nodiscard]] inline tl::optional<const Wall&> get_wall(TilePos pos) const { return m_data.get_wall(pos); }
    [[nodiscard]] inline tl::optional<Wall&> get_wall_mut(TilePos pos) { return m_data.get_wall_mut(pos); }

    [[nodiscard]] inline bool wall_exists(TilePos pos) const { return m_data.wall_exists(pos); }

    [[nodiscard]] inline Neighbors<const Wall&> get_wall_neighbors(TilePos pos) const { return m_data.get_wall_neighbors(pos); }
    [[nodiscard]] inline Neighbors<Wall&> get_wall_neighbors_mut(TilePos pos) { return m_data.get_wall_neighbors_mut(pos); }

    [[nodiscard]] inline const math::IRect& area() const { return m_data.area; }
    [[nodiscard]] inline const math::IRect& playable_area() const { return m_data.playable_area; }
    [[nodiscard]] inline const glm::uvec2& spawn_point() const { return m_data.spawn_point; }
    [[nodiscard]] inline const Layers& layers() const { return m_data.layers; }

    [[nodiscard]] inline bool is_changed() const { return m_changed; }

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
private:
    void update_neighbors(TilePos pos);

private:
    WorldData m_data;
    ChunkManager m_chunk_manager;
    std::vector<BlockDigAnimation> m_block_dig_animations;
    bool m_changed = false;
};

#endif