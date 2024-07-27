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

    inline size_t get_tile_index(TilePos pos) const {
        return (pos.y * this->area.width()) + pos.x;
    }
    
    inline bool is_tilepos_valid(TilePos pos) const {
        return (pos.x >= 0 && pos.y >= 0 && pos.x < this->area.width() && pos.y < this->area.height());
    }

    tl::optional<const Block&> get_block(TilePos pos) const {
        if (!is_tilepos_valid(pos)) return tl::nullopt;

        const size_t index = this->get_tile_index(pos);

        if (this->blocks[index].is_none()) return tl::nullopt;

        return this->blocks[index].value();
    }

    tl::optional<const Wall&> get_wall(TilePos pos) const {
        if (!is_tilepos_valid(pos)) return tl::nullopt;

        const size_t index = this->get_tile_index(pos);

        if (this->walls[index].is_none()) return tl::nullopt;
        
        return this->walls[index].value();
    }

    tl::optional<Block&> get_block_mut(TilePos pos) {
        if (!is_tilepos_valid(pos)) return tl::nullopt;

        const size_t index = this->get_tile_index(pos);

        if (this->blocks[index].is_none()) return tl::nullopt;

        return this->blocks[index].value();
    }

    tl::optional<Wall&> get_wall_mut(TilePos pos) {
        if (!is_tilepos_valid(pos)) return tl::nullopt;

        const size_t index = this->get_tile_index(pos);

        if (this->walls[index].is_none()) return tl::nullopt;
        
        return this->walls[index].value();
    }

    bool block_exists(TilePos pos) const {
        if (!is_tilepos_valid(pos)) return false;
        const size_t index = this->get_tile_index(pos);
        return this->blocks[index].is_some();
    }

    bool wall_exists(TilePos pos) const {
        if (!is_tilepos_valid(pos)) return false;
        const size_t index = this->get_tile_index(pos);
        return this->walls[index].is_some();
    }

    tl::optional<BlockType> get_block_type(TilePos pos) const {
        if (!is_tilepos_valid(pos)) return tl::nullopt;

        tl::optional<const Block&> block = get_block(pos);
        if (block.is_none()) return tl::nullopt;

        return block->type;
    }

    Neighbors<const Block&> get_block_neighbors(TilePos pos) const {
        return Neighbors<const Block&> {
            .top = this->get_block(pos.offset(TileOffset::Top)),
            .bottom = this->get_block(pos.offset(TileOffset::Bottom)),
            .left = this->get_block(pos.offset(TileOffset::Left)),
            .right = this->get_block(pos.offset(TileOffset::Right)),
            .top_left = this->get_block(pos.offset(TileOffset::TopLeft)),
            .top_right = this->get_block(pos.offset(TileOffset::TopRight)),
            .bottom_left = this->get_block(pos.offset(TileOffset::BottomLeft)),
            .bottom_right = this->get_block(pos.offset(TileOffset::BottomRight)),
        };
    }

    Neighbors<Block&> get_block_neighbors_mut(TilePos pos) {
        return Neighbors<Block&> {
            .top = this->get_block_mut(pos.offset(TileOffset::Top)),
            .bottom = this->get_block_mut(pos.offset(TileOffset::Bottom)),
            .left = this->get_block_mut(pos.offset(TileOffset::Left)),
            .right = this->get_block_mut(pos.offset(TileOffset::Right)),
            .top_left = this->get_block_mut(pos.offset(TileOffset::TopLeft)),
            .top_right = this->get_block_mut(pos.offset(TileOffset::TopRight)),
            .bottom_left = this->get_block_mut(pos.offset(TileOffset::BottomLeft)),
            .bottom_right = this->get_block_mut(pos.offset(TileOffset::BottomRight)),
        };
    }

    Neighbors<const Wall&> get_wall_neighbors(TilePos pos) const {
        return Neighbors<const Wall&> {
            .top = this->get_wall(pos.offset(TileOffset::Top)),
            .bottom = this->get_wall(pos.offset(TileOffset::Bottom)),
            .left = this->get_wall(pos.offset(TileOffset::Left)),
            .right = this->get_wall(pos.offset(TileOffset::Right)),
            .top_left = this->get_wall(pos.offset(TileOffset::TopLeft)),
            .top_right = this->get_wall(pos.offset(TileOffset::TopRight)),
            .bottom_left = this->get_wall(pos.offset(TileOffset::BottomLeft)),
            .bottom_right = this->get_wall(pos.offset(TileOffset::BottomRight)),
        };
    }

    Neighbors<Wall&> get_wall_neighbors_mut(TilePos pos) {
        return Neighbors<Wall&> {
            .top = this->get_wall_mut(pos.offset(TileOffset::Top)),
            .bottom = this->get_wall_mut(pos.offset(TileOffset::Bottom)),
            .left = this->get_wall_mut(pos.offset(TileOffset::Left)),
            .right = this->get_wall_mut(pos.offset(TileOffset::Right)),
            .top_left = this->get_wall_mut(pos.offset(TileOffset::TopLeft)),
            .top_right = this->get_wall_mut(pos.offset(TileOffset::TopRight)),
            .bottom_left = this->get_wall_mut(pos.offset(TileOffset::BottomLeft)),
            .bottom_right = this->get_wall_mut(pos.offset(TileOffset::BottomRight)),
        };
    }
    
    bool block_exists_with_type(TilePos pos, BlockType block_type) const {
        const tl::optional<BlockType> block = this->get_block_type(pos);
        if (block.is_none()) return false;

        return block.value() == block_type;
    }
};

class World {
public:
    World() {}

    void generate(uint32_t width, uint32_t height, size_t seed);

    inline tl::optional<const Block&> get_block(TilePos pos) const { return m_data.get_block(pos); }
    inline tl::optional<Block&> get_block_mut(TilePos pos) { return m_data.get_block_mut(pos); }

    inline tl::optional<BlockType> get_block_type(TilePos pos) const { return m_data.get_block_type(pos); }

    inline bool block_exists(TilePos pos) const { return m_data.block_exists(pos); }
    inline bool block_exists_with_type(TilePos pos, BlockType block_type) const { return m_data.block_exists_with_type(pos, block_type); }

    inline Neighbors<const Block&> get_block_neighbors(TilePos pos) const { return m_data.get_block_neighbors(pos); }
    inline Neighbors<Block&> get_block_neighbors_mut(TilePos pos) { return m_data.get_block_neighbors_mut(pos); }

    void set_block(TilePos pos, const Block& block);
    void set_block(TilePos pos, BlockType block_type);
    void remove_block(TilePos pos);

    void update_block_type(TilePos pos, BlockType new_type);

    inline tl::optional<const Wall&> get_wall(TilePos pos) const { return m_data.get_wall(pos); }
    inline tl::optional<Wall&> get_wall_mut(TilePos pos) { return m_data.get_wall_mut(pos); }

    inline bool wall_exists(TilePos pos) const { return m_data.wall_exists(pos); }

    inline Neighbors<const Wall&> get_wall_neighbors(TilePos pos) const { return m_data.get_wall_neighbors(pos); }
    inline Neighbors<Wall&> get_wall_neighbors_mut(TilePos pos) { return m_data.get_wall_neighbors_mut(pos); }

    void set_wall(TilePos pos, WallType wall_type);

    void update_tile_sprite_index(TilePos pos);

    void update(const Camera& camera);

    inline const math::IRect& area(void) const { return m_data.area; }
    inline const math::IRect& playable_area(void) const { return m_data.playable_area; }
    inline const glm::uvec2& spawn_point(void) const { return m_data.spawn_point; }
    inline const Layers& layers(void) const { return m_data.layers; }

    inline bool is_changed(void) const { return m_changed; }

    inline const std::unordered_map<glm::uvec2, RenderChunk>& render_chunks(void) const { return m_render_chunks; }

    inline void clear_chunks(void) { m_render_chunks.clear(); }

private:
    void manage_chunks(const Camera& camera);
    void update_neighbors(TilePos pos);

private:
    WorldData m_data;
    std::unordered_map<glm::uvec2, RenderChunk> m_render_chunks;
    bool m_changed = false;
};