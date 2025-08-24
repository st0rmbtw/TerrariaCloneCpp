#include "world.hpp"

#include <cstdint>
#include <ctime>

#include <LLGL/Tags.h>
#include <glm/glm.hpp>
#include <SGE/time/time.hpp>
#include <SGE/types/color.hpp>
#include <SGE/utils/random.hpp>
#include <SGE/profile.hpp>

#include "../types/block.hpp"
#include "../world/world_gen.h"
#include "../world/autotile.hpp"
#include "../renderer/renderer.hpp"

static void update_lightmap(WorldData& world_data, TilePos pos) {
    using Constants::LIGHT_SOLID_DECAY_STEPS;

    const glm::ivec2 size = glm::ivec2(LIGHT_SOLID_DECAY_STEPS, LIGHT_SOLID_DECAY_STEPS);

    const sge::IRect light_area = sge::IRect::from_center_half_size(pos, size).clamp(world_data.area);
    world_data.lightmap_update_area_async(light_area);
}

void World::set_block(TilePos pos, const Block& tile) {
    ZoneScoped;

    if (!m_data.is_tilepos_valid(pos)) return;

    const size_t index = m_data.get_tile_index(pos);

    if (tile.type == BlockType::Torch) {
        m_data.torches.insert(pos);
    }

    m_data.blocks[index] = tile;
    m_changed = true;
    m_lightmap_changed = true;

    update_lightmap(m_data, pos);

    m_chunk_manager.set_blocks_changed(pos);

    this->update_neighbors(pos);
}

void World::set_block(TilePos pos, BlockType tile_type) {
    ZoneScoped;

    if (!m_data.is_tilepos_valid(pos)) return;

    const size_t index = m_data.get_tile_index(pos);

    if (tile_type == BlockType::Torch) {
        m_data.torches.insert(pos);
    }

    m_data.blocks[index] = Block(tile_type);

    reset_tiles(pos, *this);

    update_neighbors(pos);

    m_changed = true;
    m_lightmap_changed = true;

    update_lightmap(m_data, pos);

    m_data.changed_tiles.emplace_back(pos, 1);

    m_chunk_manager.set_blocks_changed(pos);
}

void World::remove_block(TilePos pos) {
    ZoneScoped;

    if (!m_data.is_tilepos_valid(pos)) return;

    const size_t index = m_data.get_tile_index(pos);

    if (m_data.blocks[index].has_value()) {
        m_chunk_manager.set_blocks_changed(pos);
    }

    if (m_data.blocks[index]->type == BlockType::Torch) {
        m_data.torches.erase(pos);
    }

    m_data.blocks[index] = std::nullopt;
    m_changed = true;
    m_lightmap_changed = true;

    m_block_cracks.erase(pos);

    update_lightmap(m_data, pos);

    m_data.changed_tiles.emplace_back(pos, 0);

    reset_tiles(pos, *this);

    this->update_neighbors(pos);
}

void World::update_block(TilePos pos, BlockTypeWithData new_block, uint8_t new_variant) {
    ZoneScoped;

    if (!m_data.is_tilepos_valid(pos)) return;

    const size_t index = m_data.get_tile_index(pos);

    if (m_data.blocks[index]->type == BlockType::Torch && new_block.type != BlockType::Torch) {
        m_data.torches.erase(pos);
    }

    m_data.blocks[index]->type = new_block.type;
    m_data.blocks[index]->data = new_block.data;
    m_data.blocks[index]->variant = new_variant;

    m_changed = true;

    reset_tiles(pos, *this);
    update_neighbors(pos);
}

void World::update_block_data(TilePos pos, BlockData new_data) {
    ZoneScoped;

    if (!m_data.is_tilepos_valid(pos)) return;

    const size_t index = m_data.get_tile_index(pos);

    m_data.blocks[index]->data = new_data;

    m_changed = true;

    reset_tiles(pos, *this);
    update_neighbors(pos);
}

void World::update_block_variant(TilePos pos, uint8_t new_variant) {
    ZoneScoped;

    if (!m_data.is_tilepos_valid(pos)) return;

    const size_t index = m_data.get_tile_index(pos);

    m_data.blocks[index]->variant = new_variant;

    m_changed = true;

    reset_tiles(pos, *this);
    update_neighbors(pos);
}

void World::update_block_type(TilePos pos, BlockType new_type) {
    ZoneScoped;

    if (!m_data.is_tilepos_valid(pos)) return;

    const size_t index = m_data.get_tile_index(pos);

    m_data.blocks[index]->type = new_type;

    m_changed = true;

    reset_tiles(pos, *this);
    update_neighbors(pos);
}

void World::set_wall(TilePos pos, WallType wall_type) {
    ZoneScoped;

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
    ZoneScoped;

    if (!m_data.is_tilepos_valid(pos)) return;

    const size_t index = m_data.get_tile_index(pos);

    if (m_data.walls[index].has_value()) {
        m_chunk_manager.set_walls_changed(pos);
    }

    m_data.walls[index] = std::nullopt;
    m_changed = true;
    m_lightmap_changed = true;

    m_wall_cracks.erase(pos);

    update_lightmap(m_data, pos);

    m_data.changed_tiles.emplace_back(pos, 0);

    this->update_neighbors(pos);
}

void World::update_wall(TilePos pos, WallType new_type, uint8_t new_variant) {
    ZoneScoped;

    if (!m_data.is_tilepos_valid(pos)) return;

    const size_t index = m_data.get_tile_index(pos);
    m_data.walls[index]->type = new_type;
    m_data.walls[index]->variant = new_variant;

    m_changed = true;

    update_neighbors(pos);
}

void World::generate(uint32_t width, uint32_t height, uint32_t seed) {
    ZoneScoped;

    using Constants::WORLD_MAX_LIGHT_COUNT;

    world_generate(m_data, width, height, seed);

    m_light_count = 0;
}

void World::update(const sge::Camera& camera) {
    ZoneScoped;

    m_changed = false;
    m_lightmap_changed = false;
    m_chunk_manager.manage_chunks(m_data, camera);

    if (m_anim_timer.tick(sge::Time::Delta()).just_finished()) {
        for (glm::vec2& offset : m_offsets) {
            offset.x = sge::random::rand_int(-10, 11) * 0.15f;
            offset.y = sge::random::rand_int(-10, 1) * 0.35f;
        }
    }

    for (auto it = m_tile_dig_animations.begin(); it != m_tile_dig_animations.end();) {
        TileDigAnimation& anim = *it;

        if (anim.progress >= 1.0f) {
            it = m_tile_dig_animations.erase(it);
            continue;
        }

        anim.progress += 7.5f * sge::Time::DeltaSeconds();

        if (anim.progress >= 0.5f) {
            anim.scale = 1.0f - anim.progress;
        } else {
            anim.scale = anim.progress;
        }

        it++;
    }
}

void World::fixed_update(const sge::Rect& player_rect, Inventory& inventory) {
    m_dropped_items.update_lookup();

    for (DroppedItem& item : m_dropped_items) {
        item.update(m_data, sge::Time::FixedDeltaSeconds());
    }

    m_dropped_items.for_each_neighbor(player_rect.center(), [&](size_t index, DroppedItem& item) {
        if (item.follow_player(player_rect, inventory)) {
            inventory.add_item_stack(item.item());
            m_pickedup_item_indices.insert(index);
        }
    });

    for (auto it = m_pickedup_item_indices.begin(); it != m_pickedup_item_indices.end();) {
        m_dropped_items.remove(*it);
        it = m_pickedup_item_indices.erase(it);
    }

    m_dropped_items.update_lookup();

    stack_dropped_items();
}

void World::stack_dropped_items() {
    using Constants::ITEM_STACK_RANGE;

    for (size_t i = 0; i < m_dropped_items.size(); ++i) {
        DroppedItem& dropped_item_a = m_dropped_items[i];
        m_dropped_items.for_each_neighbor(dropped_item_a.position(), [&](size_t index, DroppedItem& dropped_item_b) {
            if (index == i) return;

            Item& item_a = dropped_item_a.item();
            Item& item_b = dropped_item_b.item();
            const ItemStack item_max_stack = item_a.max_stack;

            if (item_a.id != item_b.id)
                return;

            if (item_a.stack >= item_max_stack)
                return;

            const sge::Rect rect_b = sge::Rect::from_center_size(dropped_item_b.position(), dropped_item_b.size());
            const sge::Rect stack_rect = sge::Rect::from_center_size(dropped_item_a.position(), glm::vec2(ITEM_STACK_RANGE));

            if (!stack_rect.intersects(rect_b))
                return;

            if (item_a.stack + item_b.stack > item_max_stack) {
                const ItemStack a = item_max_stack - item_a.stack;
                item_a.stack += a;
                item_b.stack -= a;
            } else {
                item_a.stack += item_b.stack;
                item_b.stack = 0;
            }

            dropped_item_a.set_velocity((dropped_item_a.velocity() + dropped_item_b.velocity()) * 0.5f);
            dropped_item_a.set_position((dropped_item_a.position() + dropped_item_b.position()) * 0.5f);

            m_pickedup_item_indices.insert(index);
        });
    }
}

void World::draw(const sge::Camera& camera) const {
    ZoneScoped;

    sge::TextureAtlasSprite flames(Assets::GetTextureAtlas(TextureAsset::Flames0));
    flames.set_anchor(sge::Anchor::TopLeft);
    flames.set_z(0.4f);
    flames.set_index(0, 0);
    flames.set_color(sge::LinearRgba(150, 150, 150, 7));

    GameRenderer::BeginOrderMode();
        for (const TilePos& tile_pos : m_data.torches) {
            for (const glm::vec2& offset : m_offsets) {
                const glm::vec2 flame_pos = tile_pos.to_world_pos() - glm::vec2((20.0f - 16.0f) / 2.0f, 0.0f);
                flames.set_position(flame_pos + offset);
                GameRenderer::DrawAtlasSpriteWorldPremultiplied(flames);
            }
        }
    GameRenderer::EndOrderMode();

    for (const auto& [pos, cracks] : m_block_cracks) {
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
            const float zoom = glm::max(camera.zoom(), 0.6f);
            const glm::vec2 scale = glm::vec2(1.0f + anim.scale * 0.8f * zoom);
            const glm::vec2 position = anim.tile_pos.to_world_pos_center();

            sge::TextureAtlasSprite sprite(Assets::GetTextureAtlas(block_texture_asset(anim.block)), position, scale);
            sprite.set_index(anim.atlas_pos.x, anim.atlas_pos.y);

            GameRenderer::DrawAtlasSpriteWorld(sprite);

            const auto cracks = m_block_cracks.find(anim.tile_pos);
            if (cracks != m_block_cracks.end()) {
                sge::TextureAtlasSprite cracks_sprite(Assets::GetTextureAtlas(TextureAsset::TileCracks), position, scale);
                cracks_sprite.set_index(cracks->second.cracks_index);

                GameRenderer::DrawAtlasSpriteWorld(cracks_sprite, sge::Order(1));
            }
        }
    GameRenderer::EndOrderMode();

    GameRenderer::BeginOrderMode();
        for (const DroppedItem& item : m_dropped_items) {
            item.draw();
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
    Block* tile = this->get_block_mut(pos);
    Wall* wall = this->get_wall_mut(pos);

    if (tile) {
        const Neighbors<Block> neighbors = this->get_block_neighbors(pos);

        update_block_sprite_index(*tile, neighbors);

        m_chunk_manager.set_blocks_changed(pos);
    }

    if (wall) {
        const Neighbors<Wall> neighbors = this->get_wall_neighbors(pos);

        update_wall_sprite_index(*wall, neighbors);

        m_chunk_manager.set_walls_changed(pos);
    }
}
