#ifndef TERRARIA_WORLD_WORLD_DATA
#define TERRARIA_WORLD_WORLD_DATA

#pragma once

#include "../types/block.hpp"
#include "../types/wall.hpp"
#include "../types/tile_pos.hpp"
#include "../types/neighbors.hpp"

#include "../math/rect.hpp"

#include "lightmap.hpp"

struct Layers {
    int surface;
    int underground;
    int cavern;
    int dirt_height;
};

struct WorldData {
    tl::optional<Block>* blocks;
    tl::optional<Wall>* walls;
    LightMap lightmap;
    math::IRect area;
    math::IRect playable_area;
    Layers layers;
    glm::uvec2 spawn_point;
    std::vector<LightMapTask> lightmap_tasks;

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

    void lightmap_update_area_async(math::IRect area);
    void lightmap_blur_area_sync(math::IRect area);
    void lightmap_init_area(math::IRect area);
    void lightmap_init_tile(TilePos pos);

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