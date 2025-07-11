#include "world_gen.h"

#include <stdint.h>
#include <vector>
#include <FastNoiseLite/FastNoiseLite.hpp>
#include <time.h>

#include <SGE/log.hpp>
#include "../types/wall.hpp"
#include "../math/math.hpp"

#include "autotile.hpp"

using Constants::SUBDIVISION;

static constexpr int DIRT_HILL_HEIGHT = 100;

static inline void set_block(WorldData& world, TilePos pos, TileType block) {
    const size_t index = world.get_tile_index(pos);
    world.blocks[index] = Tile(block);
}

static inline void remove_block(WorldData& world, TilePos pos) {
    const size_t index = world.get_tile_index(pos);
    world.blocks[index] = std::nullopt;
}

static inline void set_wall(WorldData& world, TilePos pos, const Wall& wall) {
    const size_t index = world.get_tile_index(pos);
    world.walls[index] = wall;
}

static inline void remove_wall(WorldData& world, TilePos pos) {
    const size_t index = world.get_tile_index(pos);
    world.walls[index] = std::nullopt;
}

static void update_tile_sprite_index(WorldData& world, const TilePos& pos) {
    if (!world.is_tilepos_valid(pos)) return;

    std::optional<Tile>& block = world.blocks[world.get_tile_index(pos)];
    std::optional<Wall>& wall = world.walls[world.get_tile_index(pos)];

    if (block.has_value()) {
        const Neighbors<Tile> neighbors = world.get_tile_neighbors(pos);
        update_block_sprite_index(block.value(), neighbors);
    }

    if (wall.has_value()) {
        const Neighbors<Wall> neighbors = world.get_wall_neighbors(pos);
        update_wall_sprite_index(wall.value(), neighbors);
    }
}

static void fill_line_vertical(WorldData& world, TileType block, int from_y, int to_y, int x) {
    if (from_y < to_y) {
        for (int y = from_y; y < to_y; ++y) {
            set_block(world, {x, y}, block);
        }
    } else {
        for (int y = from_y; y > to_y; --y) {
            set_block(world, {x, y}, block);
        }
    }
}

static int get_surface_block(const WorldData &world, int x) {
    const int height = world.playable_area.height();

    int y = world.playable_area.min.y;

    while (y < height) {
        if (world.solid_tile_exists({x, y})) break;
        ++y;
    }

    return y;
}

static int get_surface_wall(const WorldData &world, int x) {
    const int max_y = world.playable_area.max.y;

    int y = world.playable_area.min.y;

    while (y < max_y) {
        if (world.wall_exists({x, y})) break;
        ++y;
    }

    return y;
}

static void world_generate_terrain(WorldData& world) {
    for (int y = world.playable_area.min.y; y < world.playable_area.max.y; ++y) {
        for (int x = world.playable_area.min.x; x < world.playable_area.max.x; ++x) {
            if (y >= world.layers.underground) {
                set_block(world, {x, y}, TileType::Stone);
            } else if (y >= world.layers.underground - world.layers.dirt_height) {
                set_block(world, {x, y}, TileType::Dirt);
            }
        }
    }
}

static void world_generate_walls(WorldData& world) {
    const int dirt_level = world.layers.underground - world.layers.dirt_height - DIRT_HILL_HEIGHT;
    const int underground_level = world.layers.underground;

    for (int y = dirt_level; y < underground_level; ++y) {
        for (int x = world.playable_area.min.x; x < world.playable_area.max.x; ++x) {
            set_wall(world, {x, y}, Wall(WallType::DirtWall));
        }
    }
}

static bool tile_pos_in_bounds(WorldData& world, TilePos pos) {
    if (pos.x <= world.playable_area.min.x || pos.x >= world.playable_area.max.x - 1) return false;
    if (pos.x <= world.playable_area.min.y || pos.y >= world.playable_area.max.y - 1) return false;
    return true;
}

static bool remove_walls_is_valid(WorldData& world, TilePos pos) {
    if (!tile_pos_in_bounds(world, pos)) return false;

    if (!world.wall_exists(pos)) return false;

    const Neighbors<Tile> neighbors = world.get_tile_neighbors(pos);
    if (neighbors.any_not_exists()) return true;

    if (world.tile_exists(pos)) return false;

    return true;
}

static void remove_walls_flood_fill(WorldData& world, TilePos start) {
    std::vector<std::pair<TilePos, glm::ivec2>> queue;
    queue.emplace_back(start, glm::ivec2(0));

    remove_wall(world, start);

    while (!queue.empty()) {
        const std::pair<TilePos, glm::ivec2>& pair = queue.back();
        const TilePos pos = pair.first;
        const glm::ivec2 depth = pair.second;

        queue.pop_back();

        {
            const TilePos new_pos = pos.offset(TileOffset::Right);
            const glm::ivec2 new_depth = depth + glm::ivec2(1, 0);
            if (remove_walls_is_valid(world, new_pos)) {
                queue.emplace_back(new_pos, new_depth);
            }
            if (tile_pos_in_bounds(world, new_pos)) {
                remove_wall(world, new_pos);
            }
        }
        {
            const TilePos new_pos = pos.offset(TileOffset::Left);
            const glm::ivec2 new_depth = depth + glm::ivec2(-1, 0);
            if (remove_walls_is_valid(world, new_pos)) {
                queue.emplace_back(new_pos, new_depth);
            }
            if (tile_pos_in_bounds(world, new_pos)) {
                remove_wall(world, new_pos);
            }
        }

        if (glm::abs(depth.x) >= depth.y / 2 + 5) continue;

        {
            const TilePos new_pos = pos.offset(TileOffset::Top);
            const glm::ivec2 new_depth = depth + glm::ivec2(0, -1);
            if (remove_walls_is_valid(world, new_pos)) {
                queue.emplace_back(new_pos, new_depth);
            }
            if (tile_pos_in_bounds(world, new_pos)) {
                remove_wall(world, new_pos);
            }
        }
        {
            const TilePos new_pos = pos.offset(TileOffset::Bottom);
            const glm::ivec2 new_depth = depth + glm::ivec2(0, 1);
            if (remove_walls_is_valid(world, new_pos)) {
                queue.emplace_back(new_pos, new_depth);
            }
            if (tile_pos_in_bounds(world, new_pos)) {
                remove_wall(world, new_pos);
            }
        }
        {
            const TilePos new_pos = pos.offset(TileOffset::TopLeft);
            const glm::ivec2 new_depth = depth + glm::ivec2(-1, -1);
            if (remove_walls_is_valid(world, new_pos)) {
                remove_wall(world, new_pos);
                queue.emplace_back(new_pos, new_depth);
            }
        }
        {
            const TilePos new_pos = pos.offset(TileOffset::TopRight);
            const glm::ivec2 new_depth = depth + glm::ivec2(1, -1);
            if (remove_walls_is_valid(world, new_pos)) {
                remove_wall(world, new_pos);
                queue.emplace_back(new_pos, new_depth);
            }
        }
        {
            const TilePos new_pos = pos.offset(TileOffset::BottomLeft);
            const glm::ivec2 new_depth = depth + glm::ivec2(-1, 1);
            if (remove_walls_is_valid(world, new_pos)) {
                remove_wall(world, new_pos);
                queue.emplace_back(new_pos, new_depth);
            }
        }
        {
            const TilePos new_pos = pos.offset(TileOffset::BottomRight);
            const glm::ivec2 new_depth = depth + glm::ivec2(1, 1);
            if (remove_walls_is_valid(world, new_pos)) {
                remove_wall(world, new_pos);
                queue.emplace_back(new_pos, new_depth);
            }
        }
    }
}

static void world_remove_walls_from_surface(WorldData& world) {
    const int min_x = world.playable_area.min.x;
    const int max_x = world.playable_area.max.x;

    for (int x = min_x; x <= max_x; ++x) {
        const int y = get_surface_wall(world, x);

        const TilePos pos = TilePos(x, y);

        if (world.tile_exists(pos)) continue;

        remove_walls_flood_fill(world, pos);
    }

    for (int x = 0; x <= max_x; ++x) {
        int y = world.playable_area.min.y;
        while (y < world.playable_area.max.y) {
            remove_wall(world, TilePos(x, y));
            if (world.tile_exists(TilePos(x, y))) break;
            y++;
        }
    }
}

static void world_make_hills(WorldData& world) {
    const int level = world.layers.underground - world.layers.dirt_height;

    FastNoiseLite fbm;
    fbm.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    fbm.SetFractalType(FastNoiseLite::FractalType_FBm);
    fbm.SetFractalOctaves(3);
    fbm.SetFractalGain(2.0);
    fbm.SetFractalLacunarity(0.5);
    fbm.SetFrequency(0.005);
    fbm.SetSeed(rand());

    // FastNoiseLite gradient;
    // gradient.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    // gradient.SetFractalType(FastNoiseLite::FractalType_FBm);
    // gradient.SetFractalOctaves(3);
    // gradient.SetFrequency(0.05);
    // gradient.SetFractalGain(2.5);
    // gradient.SetFractalLacunarity(0.48);
    // gradient.SetSeed(rand());

    FastNoiseLite gradient;
    gradient.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    gradient.SetFractalType(FastNoiseLite::FractalType_FBm);
    gradient.SetFrequency(0.045);
    gradient.SetFractalOctaves(3);
    gradient.SetFractalLacunarity(0.4);
    gradient.SetFractalGain(2.7);
    gradient.SetFractalWeightedStrength(-1.);
    gradient.SetSeed(rand());

    const int min_x = world.playable_area.min.x;
    const int max_x = world.playable_area.max.x;

    for (int x = min_x; x < max_x; ++x) {
        const float coord = static_cast<float>(x - min_x);
        const float fbm_value = fbm.GetNoise(coord, 0.0f) * 0.5 + 0.5;
        const float gradient_value = gradient.GetNoise(coord, 0.0f) * 0.5 + 0.5;
        const float noise_value = fbm_value * gradient_value;
        const int hill_height = level - (noise_value * DIRT_HILL_HEIGHT);

        fill_line_vertical(world, TileType::Dirt, hill_height, level, x);
    }
}

static void world_small_caves(WorldData& world, int seed) {
    const int underground = world.layers.underground;
    const int dirt_level = world.layers.underground - world.layers.dirt_height - DIRT_HILL_HEIGHT;

    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetSeed(seed);
    noise.SetFractalOctaves(1);
    noise.SetFrequency(0.05);

    const int min_x = world.playable_area.min.x;
    const int max_x = world.playable_area.max.x;
    const int max_y = world.playable_area.max.y;

    for (int y = dirt_level; y < max_y; ++y) {
        for (int x = min_x; x < max_x; ++x) {
            const float noise_value = noise.GetNoise(static_cast<float>(x), static_cast<float>(y));

            if (noise_value < -0.5) {
                remove_block(world, {x, y});
            }
        }
    }

    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetSeed(seed);
    noise.SetFractalOctaves(3);
    noise.SetFractalLacunarity(2.5);
    noise.SetFractalGain(0.65);
    noise.SetFrequency(0.05);

    for (int y = underground; y < max_y; ++y) {
        for (int x = min_x; x < max_x; ++x) {
            const float noise_value = noise.GetNoise(static_cast<float>(x), static_cast<float>(y));

            if (noise_value < -0.5) {
                remove_block(world, {x, y});
            }
        }
    }
}

static void world_big_caves(WorldData& world, int seed) {
    const int dirt_level = world.layers.underground - world.layers.dirt_height - DIRT_HILL_HEIGHT;

    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetSeed(seed);
    noise.SetFractalOctaves(3);
    noise.SetFractalGain(0.0);
    noise.SetFractalLacunarity(0.0);
    noise.SetFrequency(0.06);

    const int min_x = world.playable_area.min.x;
    const int max_x = world.playable_area.max.x;
    const int max_y = world.playable_area.max.y;

    for (int y = dirt_level; y < max_y; ++y) {
        for (int x = min_x; x < max_x; ++x) {
            const float noise_value = noise.GetNoise(static_cast<float>(x), static_cast<float>(y));

            if (noise_value < -0.4) {
                remove_block(world, {x, y});
            }
        }
    }
}

static void world_rough_cavern_layer_border(WorldData& world) {
    const int level = world.layers.underground;

    FastNoiseLite fbm;
    fbm.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    fbm.SetFractalType(FastNoiseLite::FractalType_FBm);
    fbm.SetFrequency(0.15);
    fbm.SetFractalOctaves(3);
    fbm.SetFractalLacunarity(0.5);
    fbm.SetFractalGain(1.5);
    fbm.SetFractalWeightedStrength(-2.);
    fbm.SetSeed(rand());

    constexpr float ROUGHNESS = 10.;
    const int min_x = world.playable_area.min.x;
    const int max_x = world.playable_area.max.x;

    for (int x = min_x; x < max_x; ++x) {
        const float noise_value = fbm.GetNoise(static_cast<float>(x - min_x), 0.0f);

        const int height = glm::abs(noise_value) * ROUGHNESS;

        if (noise_value > 0) {
            fill_line_vertical(world, TileType::Dirt, level, level + height, x);
        } else {
            fill_line_vertical(world, TileType::Stone, level - height, level, x);
        }
    }
}

static void world_generate_rocks_in_dirt(WorldData& world, int seed) {
    const int dirt_level = world.layers.underground - world.layers.dirt_height - DIRT_HILL_HEIGHT;
    const int underground_level = world.layers.underground;

    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetSeed(seed);
    noise.SetFractalOctaves(3);
    noise.SetFrequency(0.15);
    noise.SetFractalGain(0.4);
    noise.SetFractalLacunarity(2.);

    const int min_x = world.playable_area.min.x;
    const int max_x = world.playable_area.max.x;

    for (int y = dirt_level; y < underground_level; ++y) {
        for (int x = min_x; x < max_x; ++x) {
            const float noise_value = noise.GetNoise(static_cast<float>(x), static_cast<float>(y));

            if (noise_value >= 0.4) {
                auto pos = TilePos(x, y);
                const std::optional<TileType> tile = world.get_tile_type(pos);
                if (!tile.has_value()) continue;

                if (tile.value() == TileType::Dirt || tile.value() == TileType::Grass) {
                    remove_block(world, pos);
                    set_block(world, pos, TileType::Stone);
                }
            }
        }
    }
}

static void world_generate_dirt(WorldData& world, int seed, int from, int to, float noise_freq, float from_freq, float to_freq) {
    const int min_x = world.playable_area.min.x;
    const int max_x = world.playable_area.max.x;

    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFrequency(noise_freq);
    noise.SetSeed(seed);
    noise.SetFractalGain(2.0);
    noise.SetFractalLacunarity(0.5);
    noise.SetFractalOctaves(3);
    noise.SetFractalWeightedStrength(-0.63);

    for (int y = from; y < to; ++y) {
        for (int x = min_x; x < max_x; ++x) {
            const float f = map_range(static_cast<float>(from), static_cast<float>(to), from_freq, to_freq, static_cast<float>(y));

            const float noise_value = noise.GetNoise(static_cast<float>(x), static_cast<float>(y));

            if (noise_value >= f) {
                const TilePos pos = TilePos(x, y);
                if (world.tile_exists_with_type(pos, TileType::Stone)) {
                    set_block(world, pos, TileType::Dirt);
                }
            }
        }
    }
}

static void world_generate_dirt_in_rocks(WorldData& world, int seed) {
    const int underground = world.layers.underground;
    const int cavern = world.layers.cavern;

    world_generate_dirt(world, seed, underground, cavern, 0.4, 0.3, 0.7);
    world_generate_dirt(world, seed, underground, world.area.height(), 0.7, 0.5, 0.5);
}

static glm::ivec2 world_get_spawn_point(const WorldData &world) {
    const int x = world.playable_area.min.x + world.playable_area.width() / 2;
    const int y = get_surface_block(world, x);

    return {x, y};
}

static void world_update_tile_sprite_index(WorldData& world) {
    for (int y = 0; y < world.area.height(); ++y) {
        for (int x = 0; x < world.area.width(); ++x) {
            update_tile_sprite_index(world, {x, y});
        }
    }

    for (int y = 0; y < world.area.height(); ++y) {
        for (int x = 0; x < world.area.width(); ++x) {
            update_tile_sprite_index(world, {x, y});
        }
    }
}

static bool grassify_is_valid(const WorldData& world, const TilePos& pos) {
    if (pos.x >= world.area.width()) return false;
    if (pos.y >= world.area.height()) return false;
    if (!world.tile_exists_with_type(pos, TileType::Dirt)) return false;

    const Neighbors<Tile> neighbors = world.get_tile_neighbors(pos);

    return neighbors.any_not_exists();
}

static void grassify_flood_fill(WorldData &world, TilePos start) {
    std::vector<TilePos> queue;
    queue.push_back(start);

    set_block(world, start, TileType::Grass);

    while (!queue.empty()) {
        const TilePos pos = queue.back();
        queue.pop_back();

        static const TileOffset DIRECTIONS[] = {
            TileOffset::Right, TileOffset::Left, TileOffset::Top, TileOffset::Bottom, TileOffset::TopLeft, TileOffset::TopRight, TileOffset::BottomLeft, TileOffset::BottomRight
        };

        for (const TileOffset offset : DIRECTIONS) {
            const TilePos new_pos = pos.offset(offset);
            if (grassify_is_valid(world, new_pos)) {
                set_block(world, new_pos, TileType::Grass);
                queue.push_back(new_pos);
            }
        }
    }
}

static void world_grassify(WorldData& world) {
    for (int x = 0; x < world.area.width(); ++x) {
        const int y = get_surface_block(world, x);
        const TilePos pos = TilePos(x, y);
        if (world.tile_exists_with_type(pos, TileType::Dirt)) {
            grassify_flood_fill(world, pos);
        }
    }
}

static void world_generate_lightmap(WorldData& world) {
    world.lightmap_init_area(world.area);
    world.lightmap_blur_area_sync(world.area);
}

void world_generate(WorldData& world, uint32_t width, uint32_t height, uint32_t seed) {
    world.destroy();

    srand(seed);

    const sge::IRect area = sge::IRect::from_corners(glm::vec2(0), glm::ivec2(width, height) + glm::ivec2(16));
    const sge::IRect playable_area = area.inset(-8);

    const int surface_level = playable_area.min.y;
    const int underground_level = surface_level + 350;
    const int cavern_level = underground_level + 130;
    const int dirt_height = 50;

    const Layers layers = {
        .surface = surface_level,
        .underground = underground_level,
        .cavern = cavern_level,
        .dirt_height = dirt_height
    };

    SGE_LOG_DEBUG("World Layers:");
    SGE_LOG_DEBUG("  Surface: {}", layers.surface);
    SGE_LOG_DEBUG("  Underground: {}", layers.underground);
    SGE_LOG_DEBUG("  Cavern: {}", layers.cavern);
    SGE_LOG_DEBUG("  Dirt Height: {}", layers.dirt_height);

    world.blocks = new std::optional<Tile>[area.width() * area.height()];
    world.walls = new std::optional<Wall>[area.width() * area.height()];
    world.lightmap = LightMap(area.width(), area.height());
    world.playable_area = playable_area;
    world.area = area;
    world.layers = layers;

    world_generate_terrain(world);

    world_make_hills(world);

    world_generate_walls(world);

    world_rough_cavern_layer_border(world);

    world_big_caves(world, seed);
    world_small_caves(world, seed);

    world_generate_dirt_in_rocks(world, seed);

    world_grassify(world);

    world_generate_rocks_in_dirt(world, seed);

    world_remove_walls_from_surface(world);

    world_update_tile_sprite_index(world);

    world_generate_lightmap(world);

    world.spawn_point = world_get_spawn_point(world);

    srand(time(NULL));
};