#include "world.hpp"

#include <cstdint>
#include <glm/glm.hpp>
#include <ctime>

#include <tracy/Tracy.hpp>

#include <SGE/time/time.hpp>
#include <SGE/types/color.hpp>

#include "../types/block.hpp"
#include "../world/world_gen.h"
#include "../world/autotile.hpp"
#include "../renderer/renderer.hpp"

static void update_lightmap(WorldData& world_data, TilePos pos) {
    using Constants::LIGHT_SOLID_DECAY_STEPS;

    const glm::ivec2 origin = glm::vec2(pos.x, pos.y);
    const glm::ivec2 size = glm::ivec2(LIGHT_SOLID_DECAY_STEPS, LIGHT_SOLID_DECAY_STEPS);

    const sge::IRect light_area = sge::IRect::from_center_half_size(origin, size).clamp(world_data.area);
    world_data.lightmap_update_area_async(light_area);
}

void World::set_tile(TilePos pos, const Tile& tile) {
    ZoneScopedN("World::set_tile");

    if (!m_data.is_tilepos_valid(pos)) return;

    const size_t index = m_data.get_tile_index(pos);

    if (tile.type == TileType::Torch) {
        m_data.torches.insert(pos);
    }

    m_data.blocks[index] = tile;
    m_changed = true;
    m_lightmap_changed = true;

    update_lightmap(m_data, pos);

    m_chunk_manager.set_blocks_changed(pos);

    this->update_neighbors(pos);
}

void World::set_tile(TilePos pos, TileType tile_type) {
    ZoneScopedN("World::set_tile");

    if (!m_data.is_tilepos_valid(pos)) return;

    const size_t index = m_data.get_tile_index(pos);

    if (tile_type == TileType::Torch) {
        m_data.torches.insert(pos);
    }

    m_data.blocks[index] = Tile(tile_type);

    reset_tiles(pos, *this);

    update_neighbors(pos);
    
    m_changed = true;
    m_lightmap_changed = true;

    update_lightmap(m_data, pos);

    m_data.changed_tiles.emplace_back(pos, 1);

    m_chunk_manager.set_blocks_changed(pos);
}

void World::remove_tile(TilePos pos) {
    ZoneScopedN("World::remove_tile");

    if (!m_data.is_tilepos_valid(pos)) return;

    const size_t index = m_data.get_tile_index(pos);
    
    if (m_data.blocks[index].has_value()) {
        m_chunk_manager.set_blocks_changed(pos);
    }

    if (m_data.blocks[index]->type == TileType::Torch) {
        m_data.torches.erase(pos);
    }

    m_data.blocks[index] = std::nullopt;
    m_changed = true;
    m_lightmap_changed = true;

    update_lightmap(m_data, pos);

    m_data.changed_tiles.emplace_back(pos, 0);

    reset_tiles(pos, *this);

    this->update_neighbors(pos);
}

void World::update_tile(TilePos pos, TileType new_type, uint8_t new_variant) {
    ZoneScopedN("World::update_tile");

    if (!m_data.is_tilepos_valid(pos)) return;

    const size_t index = m_data.get_tile_index(pos);

    if (m_data.blocks[index]->type == TileType::Torch && new_type != TileType::Torch) {
        m_data.torches.erase(pos);
    }

    m_data.blocks[index]->type = new_type;
    m_data.blocks[index]->variant = new_variant;

    m_changed = true;

    reset_tiles(pos, *this);
    update_neighbors(pos);
}

void World::set_wall(TilePos pos, WallType wall_type) {
    ZoneScopedN("World::set_wall");

    if (!m_data.is_tilepos_valid(pos)) return;

    const size_t index = m_data.get_tile_index(pos);

    m_data.walls[index] = Wall(wall_type);
    m_changed = true;
    m_lightmap_changed = true;

    update_lightmap(m_data, pos);

    update_neighbors(pos);

    m_chunk_manager.set_walls_changed(pos);
}

void World::remove_wall(TilePos pos) {
    ZoneScopedN("World::remove_wall");

    if (!m_data.is_tilepos_valid(pos)) return;

    const size_t index = m_data.get_tile_index(pos);
    
    if (m_data.walls[index].has_value()) {
        m_chunk_manager.set_walls_changed(pos);
    }

    m_data.walls[index] = std::nullopt;
    m_changed = true;
    m_lightmap_changed = true;

    update_lightmap(m_data, pos);

    m_data.changed_tiles.emplace_back(pos, 0);

    this->update_neighbors(pos);
}

void World::update_wall(TilePos pos, WallType new_type, uint8_t new_variant) {
    ZoneScopedN("World::update_wall");

    if (!m_data.is_tilepos_valid(pos)) return;

    const size_t index = m_data.get_tile_index(pos);
    m_data.walls[index]->type = new_type;
    m_data.walls[index]->variant = new_variant;

    m_changed = true;

    update_neighbors(pos);
}

void World::generate(uint32_t width, uint32_t height, uint32_t seed) {
    ZoneScopedN("World::generate");

    using Constants::WORLD_MAX_LIGHT_COUNT;

    world_generate(m_data, width, height, seed);

    m_light_count = 0;
    if (m_lights == nullptr) {
        m_lights = new Light[WORLD_MAX_LIGHT_COUNT];
    }
}

void World::update(const sge::Camera& camera) {
    ZoneScopedN("World::update");

    m_changed = false;
    m_lightmap_changed = false;
    m_chunk_manager.manage_chunks(m_data, camera);

    if (m_anim_timer.tick(sge::Time::Delta()).just_finished()) {
        for (glm::vec2& offset : offsets) {
            offset.x = rand_int(-10, 11) * 0.15f;
            offset.y = rand_int(-10, 1) * 0.35f;
        }
    }

    for (auto it = m_tile_dig_animations.begin(); it != m_tile_dig_animations.end();) {
        TileDigAnimation& anim = *it;

        if (anim.progress >= 1.0f) {
            it = m_tile_dig_animations.erase(it);
            continue;
        }

        anim.progress += 7.0f * sge::Time::DeltaSeconds();

        if (anim.progress >= 0.5f) {
            anim.scale = 1.0f - anim.progress;
        } else {
            anim.scale = anim.progress;
        }

        it++;
    }
}

void World::draw() const {
    ZoneScopedN("World::draw");

    sge::TextureAtlasSprite flames(Assets::GetTextureAtlas(TextureAsset::Flames0));
    flames.set_anchor(sge::Anchor::TopLeft);
    flames.set_z(0.4f);
    flames.set_index(0, 0);
    flames.set_color(sge::LinearRgba(150, 150, 150, 7));

    GameRenderer::BeginOrderMode();
        for (const TilePos& tile_pos : m_data.torches) {
            for (const glm::vec2& offset : offsets) {
                float xx = offset.x;
                float yy = offset.y;
                flames.set_position(tile_pos.to_world_pos() + glm::vec2(-(20.0f - 16.0f) / 2.0f + xx, yy));
                GameRenderer::DrawAtlasSpriteWorldPremultiplied(flames);
            }
        }
    GameRenderer::EndOrderMode();

    for (const auto& [pos, cracks] : m_tile_cracks) {
        sge::TextureAtlasSprite sprite(Assets::GetTextureAtlas(TextureAsset::TileCracks));
        sprite.set_position(pos.to_world_pos_center());
        sprite.set_index(cracks.cracks_index);
        sprite.set_z(0.4f);

        GameRenderer::DrawAtlasSpriteWorld(sprite);
    }

    for (const auto& [pos, cracks] : m_wall_cracks) {
        sge::TextureAtlasSprite sprite(Assets::GetTextureAtlas(TextureAsset::TileCracks));
        sprite.set_position(pos.to_world_pos_center());
        sprite.set_index(cracks.cracks_index);
        sprite.set_z(0.2f);

        GameRenderer::DrawAtlasSpriteWorld(sprite);
    }

    GameRenderer::BeginOrderMode();
        for (const TileDigAnimation& anim : m_tile_dig_animations) {
            const glm::vec2 scale = glm::vec2(1.0f + anim.scale * 0.5f);
            const glm::vec2 position = anim.tile_pos.to_world_pos_center();

            sge::TextureAtlasSprite sprite(Assets::GetTextureAtlas(tile_texture_asset(anim.tile_type)), position, scale);
            sprite.set_index(anim.atlas_pos.x, anim.atlas_pos.y);

            GameRenderer::DrawAtlasSpriteWorld(sprite);

            const auto cracks = m_tile_cracks.find(anim.tile_pos);
            if (cracks != m_tile_cracks.end()) {
                sge::TextureAtlasSprite cracks_sprites(Assets::GetTextureAtlas(TextureAsset::TileCracks), position, scale);
                cracks_sprites.set_index(cracks->second.cracks_index);

                GameRenderer::DrawAtlasSpriteWorld(cracks_sprites);
            }
        }
    GameRenderer::EndOrderMode();
}

void World::update_neighbors(TilePos initial_pos) {
    for (int y = initial_pos.y - 3; y < initial_pos.y + 3; ++y) {
        for (int x = initial_pos.x - 3; x < initial_pos.x + 3; ++x) {
            auto pos = TilePos(x, y);
            update_tile_sprite_index(pos.offset(TileOffset::Left));
            update_tile_sprite_index(pos.offset(TileOffset::Right));
            update_tile_sprite_index(pos.offset(TileOffset::Top));
            update_tile_sprite_index(pos.offset(TileOffset::Bottom));
        }
    }
}

void World::update_tile_sprite_index(TilePos pos) {
    Tile* tile = this->get_tile_mut(pos);
    Wall* wall = this->get_wall_mut(pos);

    if (tile) {
        const Neighbors<Tile> neighbors = this->get_tile_neighbors(pos);

        update_block_sprite_index(*tile, neighbors);
    
        m_chunk_manager.set_blocks_changed(pos);
    }

    if (wall) {
        const Neighbors<Wall> neighbors = this->get_wall_neighbors(pos);

        update_wall_sprite_index(*wall, neighbors);
        
        m_chunk_manager.set_walls_changed(pos);
    }
}
