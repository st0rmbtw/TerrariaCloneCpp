#include "world.hpp"

#include <cstdint>
#include <glm/glm.hpp>
#include <ctime>

#include "../types/block.hpp"
#include "../world/world_gen.h"
#include "../world/autotile.hpp"
#include "../optional.hpp"
#include "../time/time.hpp"
#include "../renderer/renderer.hpp"

void World::set_block(TilePos pos, const Block& block) {
    if (!m_data.is_tilepos_valid(pos)) return;

    const size_t index = m_data.get_tile_index(pos);

    m_data.blocks[index] = block;
    m_changed = true;

    const math::IRect light_area = math::IRect::from_center_half_size({pos.x, pos.y}, {Constants::LightDecaySteps(), Constants::LightDecaySteps()})
        .clamp(m_data.playable_area);
    m_data.lightmap_update_area_async(light_area);

    m_chunk_manager.set_blocks_changed(pos);

    this->update_neighbors(pos);
}

void World::set_block(TilePos pos, BlockType block_type) {
    if (!m_data.is_tilepos_valid(pos)) return;

    const size_t index = m_data.get_tile_index(pos);

    m_data.blocks[index] = Block(block_type);

    reset_tiles(pos, *this);

    update_neighbors(pos);
    
    m_changed = true;

    const math::IRect light_area = math::IRect::from_center_half_size({pos.x, pos.y}, {Constants::LightDecaySteps(), Constants::LightDecaySteps()})
        .clamp(m_data.playable_area);
    m_data.lightmap_update_area_async(light_area);

    m_chunk_manager.set_blocks_changed(pos);
}

void World::remove_block(TilePos pos) {
    if (!m_data.is_tilepos_valid(pos)) return;

    const size_t index = m_data.get_tile_index(pos);
    
    if (m_data.blocks[index].is_some()) {
        m_chunk_manager.set_blocks_changed(pos);
    }

    m_data.blocks[index] = tl::nullopt;
    m_changed = true;

    const math::IRect light_area = math::IRect::from_center_half_size({pos.x, pos.y}, {Constants::LightDecaySteps(), Constants::LightDecaySteps()})
        .clamp(m_data.playable_area);
    m_data.lightmap_update_area_async(light_area);

    reset_tiles(pos, *this);

    this->update_neighbors(pos);
}

void World::update_block(TilePos pos, BlockType new_type, uint8_t new_variant) {
    const size_t index = m_data.get_tile_index(pos);
    m_data.blocks[index]->type = new_type;
    m_data.blocks[index]->variant = new_variant;
    m_changed = true;
    reset_tiles(pos, *this);
    update_neighbors(pos);
}

void World::set_wall(TilePos pos, WallType wall_type) {
    if (!m_data.is_tilepos_valid(pos)) return;

    const size_t index = m_data.get_tile_index(pos);

    m_data.walls[index] = Wall(wall_type);
    m_changed = true;

    const math::IRect light_area = math::IRect::from_center_half_size({pos.x, pos.y}, {Constants::LightDecaySteps(), Constants::LightDecaySteps()})
        .clamp(m_data.playable_area);
    m_data.lightmap_update_area_async(light_area);

    update_neighbors(pos);

    m_chunk_manager.set_walls_changed(pos);
}

void World::generate(uint32_t width, uint32_t height, uint32_t seed) {
    world_generate(m_data, width, height, seed);
}

void World::update(const Camera& camera) {
    m_changed = false;
    m_chunk_manager.manage_chunks(m_data, camera);

    for (auto it = m_block_dig_animations.begin(); it != m_block_dig_animations.end();) {
        BlockDigAnimation& anim = *it;

        if (anim.progress >= 1.0f) {
            it = m_block_dig_animations.erase(it);
            continue;
        }

        float step = 8.0f * Time::delta_seconds();

        anim.progress += step;

        if (anim.progress >= 0.5f) {
            anim.scale = 1.0f - anim.progress;
        } else {
            anim.scale = anim.progress;
        }

        it++;
    }
}

void World::draw() const {
    for (const auto& [pos, cracks] : m_tile_cracks) {
        TextureAtlasSprite sprite(Assets::GetTextureAtlas(TextureAsset::TileCracks));
        sprite.set_position(pos.to_world_pos_center());
        sprite.set_index(cracks.cracks_index);

        Renderer::DrawAtlasSprite(sprite, RenderLayer::World);
    }

    for (const BlockDigAnimation& anim : m_block_dig_animations) {
        const glm::vec2 scale = glm::vec2(1.0f + anim.scale * 0.6f);
        const glm::vec2 position = anim.tile_pos.to_world_pos_center();

        TextureAtlasSprite sprite(Assets::GetTextureAtlas(block_texture_asset(anim.block_type)), position, scale);
        sprite.set_index(anim.atlas_pos.x, anim.atlas_pos.y);

        Renderer::DrawAtlasSprite(sprite, RenderLayer::World);

        const auto cracks = m_tile_cracks.find(anim.tile_pos);
        if (cracks != m_tile_cracks.end()) {
            TextureAtlasSprite cracks_sprites(Assets::GetTextureAtlas(TextureAsset::TileCracks), position, scale);
            cracks_sprites.set_index(cracks->second.cracks_index);

            Renderer::DrawAtlasSprite(cracks_sprites, RenderLayer::World);
        }
    }
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
    tl::optional<Block&> block = this->get_block_mut(pos);
    tl::optional<Wall&> wall = this->get_wall_mut(pos);

    if (block.is_some()) {
        const Neighbors<const Block&> neighbors = this->get_block_neighbors(pos);

        update_block_sprite_index(block.get(), neighbors);
    
        m_chunk_manager.set_blocks_changed(pos);
    }

    if (wall.is_some()) {
        const Neighbors<const Wall&> neighbors = this->get_wall_neighbors(pos);

        update_wall_sprite_index(wall.get(), neighbors);
        
        m_chunk_manager.set_walls_changed(pos);
    }
}
