#include "world_data.hpp"

using Constants::SUBDIVISION;

tl::optional<const Block&> WorldData::get_block(TilePos pos) const {
    if (!is_tilepos_valid(pos)) return tl::nullopt;

    const size_t index = this->get_tile_index(pos);

    if (this->blocks[index].is_none()) return tl::nullopt;

    return this->blocks[index].get();
}

tl::optional<const Wall&> WorldData::get_wall(TilePos pos) const {
    if (!is_tilepos_valid(pos)) return tl::nullopt;

    const size_t index = this->get_tile_index(pos);

    if (this->walls[index].is_none()) return tl::nullopt;
    
    return this->walls[index].get();
}

tl::optional<Block&> WorldData::get_block_mut(TilePos pos) {
    if (!is_tilepos_valid(pos)) return tl::nullopt;

    const size_t index = this->get_tile_index(pos);

    if (this->blocks[index].is_none()) return tl::nullopt;

    return this->blocks[index].get();
}

tl::optional<Wall&> WorldData::get_wall_mut(TilePos pos) {
    if (!is_tilepos_valid(pos)) return tl::nullopt;

    const size_t index = this->get_tile_index(pos);

    if (this->walls[index].is_none()) return tl::nullopt;
    
    return this->walls[index].get();
}

tl::optional<BlockType> WorldData::get_block_type(TilePos pos) const {
    if (!is_tilepos_valid(pos)) return tl::nullopt;

    tl::optional<const Block&> block = get_block(pos);
    if (block.is_none()) return tl::nullopt;

    return block->type;
}

Neighbors<const Block&> WorldData::get_block_neighbors(TilePos pos) const {
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

Neighbors<Block&> WorldData::get_block_neighbors_mut(TilePos pos) {
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

Neighbors<const Wall&> WorldData::get_wall_neighbors(TilePos pos) const {
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

Neighbors<Wall&> WorldData::get_wall_neighbors_mut(TilePos pos) {
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

static inline float get_decay(const WorldData& world, TilePos pos) {
    if (world.block_exists(pos / SUBDIVISION)) {
        return Constants::LightDecay(true);
    } else {
        return Constants::LightDecay(false);
    }
}

static void blur(WorldData& world, TilePos pos, glm::vec3& prev_light, float& prev_decay) {
    glm::vec3 this_light = world.lightmap_get_color(pos);

    if (prev_light.x < this_light.x) {
        prev_light.x = this_light.x;
    } else {
        this_light.x = prev_light.x;
    }
    
    if (prev_light.y < this_light.y) {
        prev_light.y = this_light.y;
    } else {
        this_light.y = prev_light.y;
    }
    
    if (prev_light.z < this_light.z) {
        prev_light.z = this_light.z;
    } else {
        this_light.z = prev_light.z;
    }
    
    world.lightmap_set_color(pos, this_light);
    
    prev_light = prev_light * prev_decay;
    prev_decay = get_decay(world, pos);
}

void WorldData::lightmap_blur_area(math::IRect area) {
    const int min_y = area.min.y * SUBDIVISION;
    const int max_y = area.max.y * SUBDIVISION;

    const int min_x = area.min.x * SUBDIVISION;
    const int max_x = area.max.x * SUBDIVISION;

    for (int i = 0; i < 3; ++i) {
        // Left to right
        for (int y = min_y; y < max_y; ++y) {
            glm::vec3 prev_light = glm::vec3(0.0f);
            float prev_decay = 0.0;

            for (int x = min_x; x < max_x; ++x) {
                blur(*this, TilePos(x, y), prev_light, prev_decay);
            }
        }

        // Top to bottom
        for (int x = min_x; x < max_x; ++x) {
            glm::vec3 prev_light = glm::vec3(0.0f);
            float prev_decay = 0.0;
            for (int y = min_y; y < max_y; ++y) {
                blur(*this, TilePos(x, y), prev_light, prev_decay);
            }
        }

        // Right to left
        for (int y = min_y; y < max_y; ++y) {
            glm::vec3 prev_light = glm::vec3(0.0f);
            float prev_decay = 0.0;
            for (int x = max_x - 1; x >= min_x; --x) {
                blur(*this, TilePos(x, y), prev_light, prev_decay);
            }
        }

        // Bottom to top
        for (int x = min_x; x < max_x; ++x) {
            glm::vec3 prev_light = glm::vec3(0.0f);
            float prev_decay = 0.0;
            for (int y = max_y - 1; y >= min_x; --y) {
                blur(*this, TilePos(x, y), prev_light, prev_decay);
            }
        }
    }
}

void WorldData::lightmap_init_area(math::IRect area) {
    const int min_y = area.min.y * SUBDIVISION;
    const int max_y = area.max.y * SUBDIVISION;

    const int min_x = area.min.x * SUBDIVISION;
    const int max_x = area.max.x * SUBDIVISION;

    for (int y = min_y; y < max_y; ++y) {
        for (int x = min_x; x < max_x; ++x) {
            const auto color_pos = TilePos(x, y);

            if (y >= layers.underground * SUBDIVISION) {
                lightmap_set_color(color_pos, glm::vec3(0.0f));
                continue;
            }

            if (block_exists(color_pos / SUBDIVISION) || wall_exists(color_pos / SUBDIVISION)) {
                lightmap_set_color(color_pos, glm::vec3(0.0f));
            } else {
                lightmap_set_color(color_pos, glm::vec3(1.0f));
            }
        }
    }
}

void WorldData::lightmap_init_tile(TilePos pos) {
    const auto color_pos = TilePos(pos.x * SUBDIVISION, pos.y * SUBDIVISION);

    if (pos.y >= layers.underground) {
        lightmap_set_color(color_pos, glm::vec3(0.0f));
        return;
    }

    if (block_exists(pos) || wall_exists(pos)) {
        lightmap_set_color(color_pos, glm::vec3(0.0f));
    } else {
        lightmap_set_color(color_pos, glm::vec3(1.0f));
    }
}