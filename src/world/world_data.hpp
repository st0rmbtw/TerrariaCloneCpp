#ifndef TERRARIA_WORLD_WORLD_DATA
#define TERRARIA_WORLD_WORLD_DATA

#pragma once

#include "../types/block.hpp"
#include "../types/wall.hpp"
#include "../types/tile_pos.hpp"
#include "../types/neighbors.hpp"

#include "../math/rect.hpp"

struct Layers {
    int surface;
    int underground;
    int cavern;
    int dirt_height;
};

struct Color {
    uint8_t r, g, b, a;

    Color() : r(0), g(0), b(0), a(0) {}

    explicit Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) : r(r), g(g), b(b), a(a) {}
};

struct WorldData {
    tl::optional<Block>* blocks;
    tl::optional<Wall>* walls;
    Color* lightmap;
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
    inline glm::vec3 lightmap_get_color(TilePos pos) const {
        Color color = lightmap[pos.y * area.width() * Constants::SUBDIVISION + pos.x];
        return glm::vec3(static_cast<float>(color.r) / 255.0f, static_cast<float>(color.g) / 255.0f, static_cast<float>(color.b / 255.0f));
    }

    inline void lightmap_set_color(TilePos pos, glm::vec3 color) {
        lightmap[pos.y * area.width() * Constants::SUBDIVISION + pos.x] = Color(color.r * 255.0f, color.g * 255.0f, color.b * 255.0f, 255.0f);
    }

    [[nodiscard]]
    inline bool block_exists(TilePos pos) const {
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

    void lightmap_blur_area(math::IRect area);
    void lightmap_init_area(math::IRect area);
    void lightmap_init_tile(TilePos pos);
};

#endif