#pragma once

#ifndef WORLD_WORLD_DATA_HPP_
#define WORLD_WORLD_DATA_HPP_

#include <vector>
#include <deque>
#include <unordered_set>

#include <SGE/math/rect.hpp>

#include "../types/block.hpp"
#include "../types/wall.hpp"
#include "../types/tile_pos.hpp"
#include "../types/neighbors.hpp"

#include "lightmap.hpp"

struct Layers {
    int surface;
    int underground;
    int cavern;
    int dirt_height;
};

struct WorldData {
    std::optional<Block>* blocks;
    std::optional<Wall>* walls;
    LightMap lightmap;
    sge::IRect area;
    sge::IRect playable_area;
    Layers layers;
    glm::uvec2 spawn_point;
    std::vector<LightMapTask> lightmap_tasks;
    std::unordered_set<TilePos> torches;
    std::deque<std::pair<TilePos, int>> changed_tiles;

    [[nodiscard]]
    inline uint32_t get_tile_index(TilePos pos) const noexcept {
        return (pos.y * this->area.width()) + pos.x;
    }
    
    [[nodiscard]]
    inline bool is_tilepos_valid(TilePos pos) const noexcept {
        return (pos.x >= 0 && pos.y >= 0 && pos.x < this->area.width() && pos.y < this->area.height());
    }

    [[nodiscard]]
    std::optional<Block> get_block(TilePos pos) const noexcept {
        if (!is_tilepos_valid(pos)) return std::nullopt;
        return this->blocks[get_tile_index(pos)];
    }

    [[nodiscard]]
    std::optional<Wall> get_wall(TilePos pos) const noexcept {
        if (!is_tilepos_valid(pos)) return std::nullopt;
        return this->walls[get_tile_index(pos)];
    }

    [[nodiscard]]
    Block* get_block_mut(TilePos pos);

    [[nodiscard]]
    Wall* get_wall_mut(TilePos pos);

    [[nodiscard]]
    inline bool block_exists(TilePos pos) const noexcept {
        if (!is_tilepos_valid(pos)) return false;
        return blocks[get_tile_index(pos)].has_value();
    }

    [[nodiscard]]
    inline bool solid_block_exists(TilePos pos) const {
        if (!is_tilepos_valid(pos)) return false;

        const std::optional<Block>& tile = blocks[get_tile_index(pos)];
        if (!tile.has_value()) return false;

        return block_is_solid(tile->type);
    }

    [[nodiscard]]
    inline bool wall_exists(TilePos pos) const noexcept {
        if (!is_tilepos_valid(pos)) return false;
        return walls[this->get_tile_index(pos)].has_value();
    }

    [[nodiscard]]
    std::optional<BlockType> get_block_type(TilePos pos) const noexcept;

    [[nodiscard]]
    Neighbors<Block> get_block_neighbors(TilePos pos) const;

    [[nodiscard]]
    Neighbors<BlockType> get_block_type_neighbors(TilePos pos) const;

    [[nodiscard]]
    Neighbors<Block*> get_block_neighbors_mut(TilePos pos);

    [[nodiscard]]
    Neighbors<Wall> get_wall_neighbors(TilePos pos) const;

    [[nodiscard]]
    Neighbors<Wall*> get_wall_neighbors_mut(TilePos pos);
    
    [[nodiscard]]
    bool block_exists_with_type(TilePos pos, BlockType block_type) const noexcept {
        const std::optional<BlockType> block = this->get_block_type(pos);
        if (!block.has_value()) return false;
        return block.value() == block_type;
    }

    void lightmap_update_area_async(sge::IRect area);
    void lightmap_blur_area_sync(const sge::IRect& area);
    void lightmap_init_area(const sge::IRect& area);

    inline void lightmap_tasks_wait() {
        for (LightMapTask& task : lightmap_tasks) {
            if (task.t.joinable()) task.t.join();
        }
    }

    inline void destroy() {
        delete[] blocks;
        delete[] walls;
    }

    ~WorldData() {
        destroy();
    }
};

#endif