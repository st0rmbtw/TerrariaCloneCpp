#include "lightmap.hpp"

#include "world_data.hpp"

void LightMap::init_area(const WorldData& world, const sge::IRect& area, glm::ivec2 tile_offset) {
    ZoneScoped;

    SGE_ASSERT((area.min.x + area.width()) <= width);
    SGE_ASSERT((area.min.y + area.height()) <= height);

    using Constants::SUBDIVISION;

    for (int y = area.min.y; y < area.max.y; ++y) {
        for (int x = area.min.x; x < area.max.x; ++x) {
            const TilePos color_pos = TilePos(x, y);
            const TilePos tile_pos = tile_offset + color_pos / SUBDIVISION;

            set_mask(color_pos, world.solid_block_exists(tile_pos));

            std::optional<glm::vec3> light = block_light(world.get_block_type(tile_pos));
            if (light.has_value()) {
                set_color(color_pos, light.value());
                continue;
            }

            if (tile_offset.y * SUBDIVISION + y >= world.layers.underground * SUBDIVISION) {
                set_color(color_pos, glm::vec3(0.0f));
                continue;
            }

            if (tile_pos.x < world.playable_area.min.x || tile_pos.x > world.playable_area.max.x - 1 || world.solid_block_exists(tile_pos) || world.wall_exists(tile_pos)) {
                set_color(color_pos, glm::vec3(0.0f));
            } else {
                set_color(color_pos, glm::vec3(1.0f));
            }
        }
    }
}


uint32_t LightMap::blur_until_black(int start, int stride, glm::vec3& prev_light, float& prev_decay) {
    int index = start;
    uint32_t i = 0;
    while (i < Constants::LIGHT_AIR_DECAY_STEPS) {
        const glm::vec3 this_light = get_color(index);

        const bool x_end = prev_light.x <= this_light.x;
        const bool y_end = prev_light.y <= this_light.y;
        const bool z_end = prev_light.z <= this_light.z;

        if (x_end && y_end && z_end) {
            break;
        }

        blur(index, prev_light, prev_decay);

        index += stride;
        ++i;
    }

    return i;
}

void LightMap::blur_horizontal(const sge::IRect& area, const LightMap& reference, glm::ivec2 reference_offset) {
    SGE_ASSERT((area.min.x + area.width()) <= width);
    SGE_ASSERT((area.min.y + area.height()) <= height);

    SGE_ASSERT((reference_offset.x + area.width()) <= reference.width);
    SGE_ASSERT((reference_offset.y + area.height()) <= reference.height);

    const int min_x = std::max(reference_offset.x - 1, 1);
    const int max_x = std::min(reference_offset.x + area.width(), reference.width - 1);

    for (int y = area.min.y; y < area.max.y; ++y) {
        glm::vec3 prev_light = reference.get_color({min_x, reference_offset.y + y});
        float prev_decay = Constants::LightDecay(reference.get_mask({min_x - 1, reference_offset.y + y}));

        glm::vec3 prev_light2 = reference.get_color({max_x - 1, reference_offset.y + y});
        float prev_decay2 = Constants::LightDecay(reference.get_mask({max_x, reference_offset.y + y}));

        blur_line(y * width + area.min.x, y * width + (area.max.x - 1), 1, prev_light, prev_decay, prev_light2, prev_decay2);
    }
}

void LightMap::blur_vertical(const sge::IRect& area, const LightMap& reference, glm::ivec2 reference_offset) {
    SGE_ASSERT((area.min.x + area.width()) <= width);
    SGE_ASSERT((area.min.y + area.height()) <= height);

    SGE_ASSERT((reference_offset.x + area.width()) <= reference.width);
    SGE_ASSERT((reference_offset.y + area.height()) <= reference.height);

    const int min_y = std::max(reference_offset.y - 1, 1);
    const int max_y = std::min(reference_offset.y + area.height(), reference.height - 1);

    for (int x = area.min.x; x < area.max.x; ++x) {
        glm::vec3 prev_light = reference.get_color({reference_offset.x + x, min_y});
        float prev_decay = Constants::LightDecay(reference.get_mask({reference_offset.x + x, min_y - 1}));

        glm::vec3 prev_light2 = reference.get_color({reference_offset.x + x, max_y - 1});
        float prev_decay2 = Constants::LightDecay(reference.get_mask({reference_offset.x + x, max_y}));

        blur_line(area.min.y * width + x, (area.max.y - 1) * width + x, width, prev_light, prev_decay, prev_light2, prev_decay2);
    }
}