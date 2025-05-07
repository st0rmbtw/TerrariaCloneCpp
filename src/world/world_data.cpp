#include "world_data.hpp"

#include <cstring>
#include <memory>
#include <thread>

#include <tracy/Tracy.hpp>

#include <SGE/defines.hpp>
#include "lightmap.hpp"

using Constants::SUBDIVISION;

Tile* WorldData::get_tile_mut(TilePos pos) {
    if (!is_tilepos_valid(pos)) return nullptr;

    const size_t index = this->get_tile_index(pos);

    if (!this->blocks[index].has_value()) return nullptr;

    return &this->blocks[index].value();
}

Wall* WorldData::get_wall_mut(TilePos pos) {
    if (!is_tilepos_valid(pos)) return nullptr;

    const size_t index = this->get_tile_index(pos);

    if (!this->walls[index].has_value()) return nullptr;
    
    return &this->walls[index].value();
}

std::optional<TileType> WorldData::get_tile_type(TilePos pos) const {
    if (!is_tilepos_valid(pos)) return std::nullopt;

    const std::optional<Tile> block = get_tile(pos);
    if (!block.has_value()) return std::nullopt;

    return block->type;
}

Neighbors<Tile> WorldData::get_tile_neighbors(TilePos pos) const {
    return Neighbors<Tile> {
        .top = get_tile(pos.offset(TileOffset::Top)),
        .bottom = get_tile(pos.offset(TileOffset::Bottom)),
        .left = get_tile(pos.offset(TileOffset::Left)),
        .right = get_tile(pos.offset(TileOffset::Right)),
        .top_left = get_tile(pos.offset(TileOffset::TopLeft)),
        .top_right = get_tile(pos.offset(TileOffset::TopRight)),
        .bottom_left = get_tile(pos.offset(TileOffset::BottomLeft)),
        .bottom_right = get_tile(pos.offset(TileOffset::BottomRight)),
    };
}

Neighbors<TileType> WorldData::get_tile_type_neighbors(TilePos pos) const {
    return Neighbors<TileType> {
        .top = get_tile_type(pos.offset(TileOffset::Top)),
        .bottom = get_tile_type(pos.offset(TileOffset::Bottom)),
        .left = get_tile_type(pos.offset(TileOffset::Left)),
        .right = get_tile_type(pos.offset(TileOffset::Right)),
        .top_left = get_tile_type(pos.offset(TileOffset::TopLeft)),
        .top_right = get_tile_type(pos.offset(TileOffset::TopRight)),
        .bottom_left = get_tile_type(pos.offset(TileOffset::BottomLeft)),
        .bottom_right = get_tile_type(pos.offset(TileOffset::BottomRight)),
    };
}

Neighbors<Tile*> WorldData::get_tile_neighbors_mut(TilePos pos) {
    return Neighbors<Tile*> {
        .top = this->get_tile_mut(pos.offset(TileOffset::Top)),
        .bottom = this->get_tile_mut(pos.offset(TileOffset::Bottom)),
        .left = this->get_tile_mut(pos.offset(TileOffset::Left)),
        .right = this->get_tile_mut(pos.offset(TileOffset::Right)),
        .top_left = this->get_tile_mut(pos.offset(TileOffset::TopLeft)),
        .top_right = this->get_tile_mut(pos.offset(TileOffset::TopRight)),
        .bottom_left = this->get_tile_mut(pos.offset(TileOffset::BottomLeft)),
        .bottom_right = this->get_tile_mut(pos.offset(TileOffset::BottomRight)),
    };
}

Neighbors<Wall> WorldData::get_wall_neighbors(TilePos pos) const {
    return Neighbors<Wall> {
        .top = get_wall(pos.offset(TileOffset::Top)),
        .bottom = get_wall(pos.offset(TileOffset::Bottom)),
        .left = get_wall(pos.offset(TileOffset::Left)),
        .right = get_wall(pos.offset(TileOffset::Right)),
        .top_left = get_wall(pos.offset(TileOffset::TopLeft)),
        .top_right = get_wall(pos.offset(TileOffset::TopRight)),
        .bottom_left = get_wall(pos.offset(TileOffset::BottomLeft)),
        .bottom_right = get_wall(pos.offset(TileOffset::BottomRight)),
    };
}

Neighbors<Wall*> WorldData::get_wall_neighbors_mut(TilePos pos) {
    return Neighbors<Wall*> {
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

static void internal_lightmap_init_area(WorldData& world, LightMap& lightmap, const sge::IRect& area, glm::ivec2 tile_offset = {0, 0}) {
    ZoneScopedN("LightMap::init_area");

    const int min_y = area.min.y * SUBDIVISION;
    const int max_y = area.max.y * SUBDIVISION;

    const int min_x = area.min.x * SUBDIVISION;
    const int max_x = area.max.x * SUBDIVISION;

    #pragma omp parallel for collapse(2)
    for (int y = min_y; y < max_y; ++y) {
        for (int x = min_x; x < max_x; ++x) {
            const TilePos color_pos = TilePos(x, y);
            const TilePos tile_pos = tile_offset + color_pos / SUBDIVISION;

            lightmap.set_mask(color_pos, world.tile_exists(tile_pos));

            std::optional<glm::vec3> light = tile_light(world.get_tile_type(tile_pos));
            if (light.has_value()) {
                lightmap.set_color(color_pos, light.value());
                continue;
            }

            if (tile_offset.y * SUBDIVISION + y >= world.layers.underground * SUBDIVISION) {
                lightmap.set_color(color_pos, glm::vec3(0.0f));
                continue;
            }

            if (tile_pos.x < world.playable_area.min.x || tile_pos.x > world.playable_area.max.x - 1 || world.tile_exists(tile_pos) || world.wall_exists(tile_pos)) {
                lightmap.set_color(color_pos, glm::vec3(0.0f));
            } else {
                lightmap.set_color(color_pos, glm::vec3(1.0f));
            }
        }
    }
}

static void blur(LightMap& lightmap, int index, glm::vec3& prev_light, float& prev_decay) {
    using Constants::LIGHT_EPSILON;

    glm::vec3 this_light = lightmap.get_color(index);

    prev_light.r = prev_light.r < LIGHT_EPSILON ? 0.0f : prev_light.r;
    prev_light.g = prev_light.g < LIGHT_EPSILON ? 0.0f : prev_light.g;
    prev_light.b = prev_light.b < LIGHT_EPSILON ? 0.0f : prev_light.b;

    if (prev_light.r < this_light.r) {
        prev_light.r = this_light.r;
    } else {
        this_light.r = prev_light.r;
    }

    if (prev_light.g < this_light.g) {
        prev_light.g = this_light.g;
    } else {
        this_light.g = prev_light.g;
    }

    if (prev_light.b < this_light.b) {
        prev_light.b = this_light.b;
    } else {
        this_light.b = prev_light.b;
    }

    lightmap.set_color(index, this_light);

    prev_light = prev_light * prev_decay;
    prev_decay = Constants::LightDecay(lightmap.get_mask(index));
}

SGE_FORCE_INLINE static void blur_line(LightMap& lightmap, int start, int end, int stride, glm::vec3& prev_light, float& prev_decay, glm::vec3& prev_light2, float& prev_decay2) {
    using Constants::LIGHT_EPSILON;

    int length = end - start;
    for (int index = 0; index < length; index += stride) {
        blur(lightmap, start + index, prev_light, prev_decay);
        blur(lightmap, end - index, prev_light2, prev_decay2);
    }
}

SGE_FORCE_INLINE static void blur_horizontal(const WorldData& world, LightMap& lightmap, const sge::IRect& area, TilePos offset) {
    #pragma omp parallel for
    for (int y = area.min.y; y < area.max.y; ++y) {
        glm::vec3 prev_light = world.lightmap.get_color({offset.x + area.min.x, offset.y + y});
        float prev_decay = Constants::LightDecay(world.lightmap.get_mask({(offset.x + area.min.x) - 1, (offset.y + y)}));

        glm::vec3 prev_light2 = world.lightmap.get_color({offset.x + area.max.x - 1, offset.y + y});
        float prev_decay2 = Constants::LightDecay(world.lightmap.get_mask({(offset.x + area.max.x), (offset.y + y)}));

        blur_line(lightmap, y * lightmap.width + area.min.x, y * lightmap.width + (area.max.x - 1), 1, prev_light, prev_decay, prev_light2, prev_decay2);
    }
}

SGE_FORCE_INLINE static void blur_vertical(const WorldData& world, LightMap& lightmap, const sge::IRect& area, TilePos offset) {
    #pragma omp parallel for
    for (int x = area.min.x; x < area.max.x; ++x) {
        glm::vec3 prev_light = world.lightmap.get_color({offset.x + x, offset.y + area.min.y});
        float prev_decay = Constants::LightDecay(world.lightmap.get_mask({(offset.x + x), (offset.y + area.min.y) - 1}));

        glm::vec3 prev_light2 = world.lightmap.get_color({offset.x + x, offset.y + area.max.y - 1});
        float prev_decay2 = Constants::LightDecay(world.lightmap.get_mask({(offset.x + x), (offset.y + area.max.y)}));

        blur_line(lightmap, area.min.y * lightmap.width + x, (area.max.y - 1) * lightmap.width + x, lightmap.width, prev_light, prev_decay, prev_light2, prev_decay2);
    }
}

static void internal_lightmap_blur_area(WorldData& world, LightMap& lightmap, const sge::IRect& area, glm::ivec2 tile_offset = {0, 0}) {
    const sge::IRect lightmap_area = area * SUBDIVISION;
    const TilePos offset = {tile_offset.x * SUBDIVISION, tile_offset.y * SUBDIVISION};

    blur_horizontal(world, lightmap, lightmap_area, offset);
    blur_vertical(world, lightmap, lightmap_area, offset);

    blur_horizontal(world, lightmap, lightmap_area, offset);
    blur_vertical(world, lightmap, lightmap_area, offset);

    blur_horizontal(world, lightmap, lightmap_area, offset);
}

static void internal_lightmap_update_area_async(const std::shared_ptr<std::atomic<LightMapTaskResult>>& result, WorldData* world, const sge::IRect& area) {
    LightMap lightmap(area.width(), area.height());

    const sge::IRect a = sge::IRect::from_top_left(glm::ivec2(0), area.size());

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

void WorldData::lightmap_blur_area_sync(const sge::IRect& area) {
    internal_lightmap_blur_area(*this, this->lightmap, area);
}

void WorldData::lightmap_update_area_async(sge::IRect area) {
    std::shared_ptr<std::atomic<LightMapTaskResult>> result = std::make_shared<std::atomic<LightMapTaskResult>>();
    lightmap_tasks.emplace_back(std::thread(internal_lightmap_update_area_async, result, this, area), result);
}

void WorldData::lightmap_init_area(const sge::IRect& area) {
    internal_lightmap_init_area(*this, this->lightmap, area);
}