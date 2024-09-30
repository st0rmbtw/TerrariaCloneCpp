#include "world_data.hpp"

#include <cstring>
#include <memory>
#include <thread>

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

static void internal_lightmap_init_area(WorldData& world, LightMap& lightmap, math::IRect area, glm::ivec2 tile_offset = {0, 0}) {
    const int min_y = area.min.y * SUBDIVISION;
    const int max_y = area.max.y * SUBDIVISION;

    const int min_x = area.min.x * SUBDIVISION;
    const int max_x = area.max.x * SUBDIVISION;

    for (int y = min_y; y < max_y; ++y) {
        for (int x = min_x; x < max_x; ++x) {
            const auto color_pos = TilePos(x, y);

            if (tile_offset.y * SUBDIVISION + y >= world.layers.underground * SUBDIVISION) {
                lightmap.set_color(color_pos, glm::vec3(0.0f));
                continue;
            }

            if (world.block_exists(TilePos(tile_offset) + color_pos / SUBDIVISION) || world.wall_exists(TilePos(tile_offset) + color_pos / SUBDIVISION)) {
                lightmap.set_color(color_pos, glm::vec3(0.0f));
            } else {
                lightmap.set_color(color_pos, glm::vec3(1.0f));
            }
        }
    }
}

static inline float get_decay(const WorldData& world, TilePos pos) {
    return Constants::LightDecay(world.block_exists(pos));
}

static void blur(WorldData& world, LightMap& lightmap, TilePos pos, glm::vec3& prev_light, float& prev_decay, glm::ivec2 tile_offset) {
    glm::vec3 this_light = lightmap.get_color(pos);

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
    
    lightmap.set_color(pos, this_light);
    
    prev_light = prev_light * prev_decay;
    prev_decay = get_decay(world, TilePos(tile_offset) + pos / SUBDIVISION);
}

static void internal_lightmap_blur_area(WorldData& world, LightMap& lightmap, math::IRect area, glm::ivec2 tile_offset = {0, 0}) {
    const math::IRect lightmap_area = area * SUBDIVISION;

    const int min_x = lightmap_area.min.x;
    const int min_y = lightmap_area.min.y;
    const int max_x = lightmap_area.max.x;
    const int max_y = lightmap_area.max.y;

    const TilePos offset = {tile_offset.x * SUBDIVISION, tile_offset.y * SUBDIVISION};

    for (int i = 0; i < 3; ++i) {
        // Left to right
        #pragma omp parallel for
        for (int y = min_y; y < max_y; ++y) {
            glm::vec3 prev_light = world.lightmap.get_color({offset.x + min_x, offset.y + y});
            float prev_decay = get_decay(world, {(offset.x + min_x) / SUBDIVISION - 1, (offset.y + y) / SUBDIVISION});

            for (int x = min_x; x < max_x; ++x) {
                blur(world, lightmap, TilePos(x, y), prev_light, prev_decay, tile_offset);
            }
        }

        // Top to bottom
        #pragma omp parallel for
        for (int x = min_x; x < max_x; ++x) {
            glm::vec3 prev_light = world.lightmap.get_color({offset.x + x, offset.y + min_y});
            float prev_decay = get_decay(world, {(offset.x + x) / SUBDIVISION, (offset.y + min_y) / SUBDIVISION - 1});

            for (int y = min_y; y < max_y; ++y) {
                blur(world, lightmap, TilePos(x, y), prev_light, prev_decay, tile_offset);
            }
        }

        // Right to left
        #pragma omp parallel for
        for (int y = min_y; y < max_y; ++y) {
            glm::vec3 prev_light = world.lightmap.get_color({offset.x + max_x - 1, offset.y + y});
            float prev_decay = get_decay(world, {(offset.x + max_x) / SUBDIVISION, (offset.y + y) / SUBDIVISION});

            for (int x = max_x - 1; x >= min_x; --x) {
                blur(world, lightmap, TilePos(x, y), prev_light, prev_decay, tile_offset);
            }
        }

        // Bottom to top
        #pragma omp parallel for
        for (int x = min_x; x < max_x; ++x) {
            glm::vec3 prev_light = world.lightmap.get_color({offset.x + x, offset.y + max_y - 1});
            float prev_decay = get_decay(world, {(offset.x + x) / SUBDIVISION, (offset.y + max_y) / SUBDIVISION});

            for (int y = max_y - 1; y >= min_y; --y) {
                blur(world, lightmap, TilePos(x, y), prev_light, prev_decay, tile_offset);
            }
        }
    }
}

static void internal_lightmap_update_area_async(std::shared_ptr<std::atomic<LightMapTaskResult>> result, WorldData* world, math::IRect area) {
    LightMap lightmap = LightMap(area.width(), area.height());

    const math::IRect a = math::IRect::from_top_left(glm::ivec2(0), area.size());

    WorldData& w = *world;

    internal_lightmap_init_area(w, lightmap, a, area.min);
    internal_lightmap_blur_area(w, lightmap, a, area.min);

    *result = LightMapTaskResult {
        .data = lightmap.data,
        .width = lightmap.width,
        .height = lightmap.height,
        .offset_x = area.min.x * SUBDIVISION,
        .offset_y = area.min.y * SUBDIVISION,
        .is_complete = true
    };

    lightmap.data = nullptr;
}

void WorldData::lightmap_blur_area_sync(math::IRect area) {
    internal_lightmap_blur_area(*this, this->lightmap, area);
}

void WorldData::lightmap_update_area_async(math::IRect area) {
    std::shared_ptr<std::atomic<LightMapTaskResult>> result = std::make_shared<std::atomic<LightMapTaskResult>>();
    lightmap_tasks.emplace_back(std::thread(internal_lightmap_update_area_async, result, this, area), result);
}

void WorldData::lightmap_init_area(math::IRect area) {
    internal_lightmap_init_area(*this, this->lightmap, area);
}

void WorldData::lightmap_init_tile(TilePos pos) {
    const auto color_pos = TilePos(pos.x * SUBDIVISION, pos.y * SUBDIVISION);

    if (pos.y >= layers.underground) {
        lightmap.set_color(color_pos, glm::vec3(0.0f));
        return;
    }

    if (block_exists(pos) || wall_exists(pos)) {
        lightmap.set_color(color_pos, glm::vec3(0.0f));
    } else {
        lightmap.set_color(color_pos, glm::vec3(1.0f));
    }
}