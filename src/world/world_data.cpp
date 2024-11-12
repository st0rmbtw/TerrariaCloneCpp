#include "world_data.hpp"

#include <cstring>
#include <memory>
#include <thread>

#include "../defines.hpp"
#include "lightmap.hpp"

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

static void internal_lightmap_init_area(WorldData& world, LightMap& lightmap, const math::IRect& area, glm::ivec2 tile_offset = {0, 0}) {
    const int min_y = area.min.y * SUBDIVISION;
    const int max_y = area.max.y * SUBDIVISION;

    const int min_x = area.min.x * SUBDIVISION;
    const int max_x = area.max.x * SUBDIVISION;

    #pragma omp parallel for collapse(2)
    for (int y = min_y; y < max_y; ++y) {
        for (int x = min_x; x < max_x; ++x) {
            const TilePos color_pos = TilePos(x, y);
            const TilePos tile_pos = tile_offset + color_pos / SUBDIVISION;

            lightmap.set_mask(color_pos, world.block_exists(tile_pos));

            if (tile_offset.y * SUBDIVISION + y >= world.layers.underground * SUBDIVISION) {
                lightmap.set_color(color_pos, glm::vec3(0.0f));
                continue;
            }

            if (tile_pos.x <= world.playable_area.min.x || tile_pos.x >= world.playable_area.max.x - 1 || world.block_exists(tile_pos) || world.wall_exists(tile_pos)) {
                lightmap.set_color(color_pos, glm::vec3(0.0f));
            } else {
                lightmap.set_color(color_pos, glm::vec3(1.0f));
            }
        }
    }
}

static void blur_line(LightMap& lightmap, int start, int end, int stride, glm::vec3& prev_light, float& prev_decay) {
    using Constants::LIGHT_EPSILON;

    for (int index = start; index != end + stride; index += stride) {
        glm::vec3 this_light = lightmap.get_color(index);

        prev_light.x = prev_light.x < LIGHT_EPSILON ? 0.0f : prev_light.x;
        prev_light.y = prev_light.y < LIGHT_EPSILON ? 0.0f : prev_light.x;
        prev_light.z = prev_light.z < LIGHT_EPSILON ? 0.0f : prev_light.x;

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

        lightmap.set_color(index, this_light);

        prev_light = prev_light * prev_decay;
        prev_decay = Constants::LightDecay(lightmap.get_mask(index));
    }
}

static void internal_lightmap_blur_area(WorldData& world, LightMap& lightmap, const math::IRect& area, glm::ivec2 tile_offset = {0, 0}) {
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
            float prev_decay = Constants::LightDecay(world.lightmap.get_mask({(offset.x + min_x) - 1, (offset.y + y)}));

            blur_line(lightmap, y * lightmap.width + min_x, y * lightmap.width + (max_x - 1), 1, prev_light, prev_decay);
        }

        // Top to bottom
        #pragma omp parallel for
        for (int x = min_x; x < max_x; ++x) {
            glm::vec3 prev_light = world.lightmap.get_color({offset.x + x, offset.y + min_y});
            float prev_decay = Constants::LightDecay(world.lightmap.get_mask({(offset.x + x), (offset.y + min_y) - 1}));

            blur_line(lightmap, min_y * lightmap.width + x, (max_y - 1) * lightmap.width + x, lightmap.width, prev_light, prev_decay);
        }

        // Right to left
        #pragma omp parallel for
        for (int y = min_y; y < max_y; ++y) {
            glm::vec3 prev_light = world.lightmap.get_color({offset.x + max_x - 1, offset.y + y});
            float prev_decay = Constants::LightDecay(world.lightmap.get_mask({(offset.x + max_x), (offset.y + y)}));

            blur_line(lightmap, y * lightmap.width + (max_x - 1), y * lightmap.width + min_x, -1, prev_light, prev_decay);
        }

        // Bottom to top
        #pragma omp parallel for
        for (int x = min_x; x < max_x; ++x) {
            glm::vec3 prev_light = world.lightmap.get_color({offset.x + x, offset.y + max_y - 1});
            float prev_decay = Constants::LightDecay(world.lightmap.get_mask({(offset.x + x), (offset.y + max_y)}));

            blur_line(lightmap, (max_y - 1) * lightmap.width + x, min_y * lightmap.width + x, -lightmap.width, prev_light, prev_decay);
        }
    }
}

static void internal_lightmap_update_area_async(std::shared_ptr<std::atomic<LightMapTaskResult>> result, WorldData* world, math::IRect area) {
    LightMap lightmap(area.width(), area.height());

    const math::IRect a = math::IRect::from_top_left(glm::ivec2(0), area.size());

    internal_lightmap_init_area(*world, lightmap, a, area.min);
    internal_lightmap_blur_area(*world, lightmap, a, area.min);

    *result = LightMapTaskResult {
        .data = lightmap.colors,
        .mask = lightmap.masks,
        .width = lightmap.width,
        .height = lightmap.height,
        .offset_x = area.min.x * SUBDIVISION,
        .offset_y = area.min.y * SUBDIVISION,
        .is_complete = true
    };

    lightmap.colors = nullptr;
    lightmap.masks = nullptr;
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