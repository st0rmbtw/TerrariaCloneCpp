#ifndef WORLD_WORLD_HPP
#define WORLD_WORLD_HPP

#pragma once

#include <stdint.h>
#include <unordered_map>

#include "math/rect.hpp"
#include "renderer/camera.h"

#include "types/block.hpp"
#include "types/wall.hpp"
#include "types/tile_pos.hpp"
#include "world/chunk.hpp"

#include "optional.hpp"

struct Layers {
    int surface;
    int underground;
    int cavern;
    int dirt_height;
};

struct WorldData {
    tl::optional<Block>* blocks;
    tl::optional<Wall>* walls;
    math::IRect area;
    math::IRect playable_area;
    Layers layers;
    glm::uvec2 spawn_point;

    [[nodiscard]]
    inline size_t get_tile_index(TilePos pos) const {
        return (pos.y * this->area.width()) + pos.x;
    }
    
    [[nodiscard]]
    inline bool is_tilepos_valid(TilePos pos) const {
        return (pos.x >= 0 && pos.y >= 0 && pos.x < this->area.width() && pos.y < this->area.height());
    }

    [[nodiscard]]
    tl::optional<const Block&> get_block(TilePos pos) const;

    [[nodiscard]]
    tl::optional<const Wall&> get_wall(TilePos pos) const;

    [[nodiscard]]
    tl::optional<Block&> get_block_mut(TilePos pos);

    [[nodiscard]]
    tl::optional<Wall&> get_wall_mut(TilePos pos);

    [[nodiscard]]
    inline bool block_exists(TilePos pos) const {
        if (!is_tilepos_valid(pos)) return false;
        return blocks[this->get_tile_index(pos)].is_some();
    }

    [[nodiscard]]
    inline bool wall_exists(TilePos pos) const {
        if (!is_tilepos_valid(pos)) return false;
        return walls[this->get_tile_index(pos)].is_some();
    }

    [[nodiscard]]
    tl::optional<BlockType> get_block_type(TilePos pos) const;

    [[nodiscard]]
    Neighbors<const Block&> get_block_neighbors(TilePos pos) const;

    [[nodiscard]]
    Neighbors<Block&> get_block_neighbors_mut(TilePos pos);

    [[nodiscard]]
    Neighbors<const Wall&> get_wall_neighbors(TilePos pos) const;

    [[nodiscard]]
    Neighbors<Wall&> get_wall_neighbors_mut(TilePos pos);
    
    [[nodiscard]]
    bool block_exists_with_type(TilePos pos, BlockType block_type) const {
        const tl::optional<BlockType> block = this->get_block_type(pos);
        if (block.is_none()) return false;
        return block.value() == block_type;
    }
};

class World {
public:
    World() = default;

    void generate(uint32_t width, uint32_t height, size_t seed);


    void set_block(TilePos pos, const Block& block);
    void set_block(TilePos pos, BlockType block_type);
    void remove_block(TilePos pos);

    void update_block_type(TilePos pos, BlockType new_type);

    void set_wall(TilePos pos, WallType wall_type);

    void update_tile_sprite_index(TilePos pos);

    void update(const Camera& camera);

    void manage_chunks(const Camera& camera);

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

    [[nodiscard]] inline const std::unordered_map<glm::uvec2, RenderChunk>& render_chunks() const { return m_render_chunks; }

    inline void clear_chunks() { m_render_chunks.clear(); }
private:
    void update_neighbors(TilePos pos);

private:
    WorldData m_data;
    std::unordered_map<glm::uvec2, RenderChunk> m_render_chunks;
    bool m_changed = false;
};

#endif