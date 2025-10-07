#include "world.hpp"

#include <cstdint>
#include <ctime>

#include <LLGL/Tags.h>
#include <glm/glm.hpp>
#include <SGE/time/time.hpp>
#include <SGE/types/color.hpp>
#include <SGE/types/sprite.hpp>
#include <SGE/utils/random.hpp>
#include <SGE/profile.hpp>

#include "../types/block.hpp"
#include "../world/world_gen.h"
#include "../world/autotile.hpp"
#include "../renderer/renderer.hpp"
#include "chunk.hpp"

static constexpr int LIGHTMAP_INSET = 1;

void World::update_lightmap(TilePos pos) {
    using Constants::LIGHT_SOLID_DECAY_STEPS;
    using Constants::LIGHTMAP_CHUNK_TILE_SIZE;
    using Constants::LIGHTMAP_CHUNK_SIZE;

    const sge::IRect light_area = sge::IRect::from_center_half_size(pos, glm::ivec2(LIGHT_SOLID_DECAY_STEPS, LIGHT_SOLID_DECAY_STEPS)).clamp(m_data.area) * Constants::SUBDIVISION;

    std::shared_ptr<std::atomic<UpdateLightMapTaskData>> result = std::make_shared<std::atomic<UpdateLightMapTaskData>>();

    auto task = std::thread([this](sge::IRect light_area, const std::shared_ptr<std::atomic<UpdateLightMapTaskData>>& result) {
        UpdateLightMapTaskData task;

        const sge::IRect area = sge::IRect::from_top_left(glm::ivec2(0), light_area.size() + LIGHTMAP_INSET * 2);

        LightMap lightmap(area.size());
        lightmap.init_area(m_data, area.inset(-1), light_area.min / Constants::SUBDIVISION);

        const sge::IRect chunk_range = light_area / Constants::LIGHTMAP_CHUNK_SIZE;

        {
            int remaining_width = light_area.width();
            int offset = light_area.min.x % int(LIGHTMAP_CHUNK_SIZE);
            int write_offset = 0;
            int x_pos = chunk_range.min.x;
            while (remaining_width > 0) {
                const glm::ivec2 top_pos = glm::ivec2(x_pos, chunk_range.min.y);
                const glm::ivec2 bottom_pos = glm::ivec2(x_pos, chunk_range.max.y);

                const StaticLightMapChunk* top = m_chunk_manager.light_chunks().get_unchecked(top_pos);
                const StaticLightMapChunk* bottom = m_chunk_manager.light_chunks().get_unchecked(bottom_pos);

                const int from_x = std::max(offset % int(LIGHTMAP_CHUNK_SIZE), LIGHTMAP_INSET);
                const int width = std::min(from_x + remaining_width, top->lightmap.width) - from_x;

                if (top != nullptr) {
                    int from_y = std::clamp(
                        light_area.min.y % int(LIGHTMAP_CHUNK_SIZE),
                        LIGHTMAP_INSET, top->lightmap.height - 1 - LIGHTMAP_INSET
                    );

                    memcpy(&lightmap.colors[write_offset], &top->lightmap.colors[from_y * top->lightmap.width + from_x], width * sizeof(Color));
                    memcpy(&lightmap.masks[write_offset], &top->lightmap.masks[from_y * top->lightmap.width + from_x], width * sizeof(LightMask));
                }
                if (bottom != nullptr) {
                    int from_y = std::clamp(
                        light_area.max.y % int(LIGHTMAP_CHUNK_SIZE),
                        LIGHTMAP_INSET, bottom->lightmap.height - 1 - LIGHTMAP_INSET
                    );

                    memcpy(&lightmap.colors[(lightmap.height - 1) * lightmap.width + write_offset], &bottom->lightmap.colors[from_y * bottom->lightmap.width + from_x], width * sizeof(Color));
                    memcpy(&lightmap.masks[(lightmap.height - 1) * lightmap.width + write_offset], &bottom->lightmap.masks[from_y * bottom->lightmap.width + from_x], width * sizeof(LightMask));
                }

                remaining_width -= width;
                offset += width;
                write_offset += width;
                x_pos += 1;
            }
        }
        {
            int remaining_height = light_area.height();
            int offset = light_area.min.y;
            int write_offset = 0;
            int y_pos = chunk_range.min.y;

            while (remaining_height > 0) {
                const glm::ivec2 left_pos = glm::ivec2(chunk_range.min.x, y_pos);
                const glm::ivec2 right_pos = glm::ivec2(chunk_range.max.x, y_pos);

                const StaticLightMapChunk* left = m_chunk_manager.light_chunks().get_unchecked(left_pos);
                const StaticLightMapChunk* right = m_chunk_manager.light_chunks().get_unchecked(right_pos);

                const int from_y = std::max(offset % int(LIGHTMAP_CHUNK_SIZE), LIGHTMAP_INSET);
                const int height = std::min(from_y + remaining_height, left->lightmap.height - 1 - LIGHTMAP_INSET) - from_y;

                if (left != nullptr) {
                    int from_x = std::clamp(
                        light_area.min.x % int(LIGHTMAP_CHUNK_SIZE),
                        LIGHTMAP_INSET, left->lightmap.width - 1 - LIGHTMAP_INSET
                    );
                    for (int y = 0; y < height; ++y) {
                        lightmap.colors[(write_offset + y) * lightmap.width] = left->lightmap.colors[(from_y + y) * left->lightmap.width + from_x];
                        lightmap.masks[(write_offset + y) * lightmap.width] = left->lightmap.masks[(from_y + y) * left->lightmap.width + from_x];
                    }
                }
                if (right != nullptr) {
                    int from_x = std::clamp(
                        light_area.max.x % int(LIGHTMAP_CHUNK_SIZE),
                        LIGHTMAP_INSET, right->lightmap.width - 1 - LIGHTMAP_INSET
                    );
                    for (int y = 0; y < height; ++y) {
                        lightmap.colors[(write_offset + y) * lightmap.width + lightmap.width - 1] = right->lightmap.colors[(from_y + y) * right->lightmap.width + from_x];
                        lightmap.masks[(write_offset + y) * lightmap.width + lightmap.width - 1] = right->lightmap.masks[(from_y + y) * right->lightmap.width + from_x];
                    }
                }
                remaining_height -= height;
                offset += height;
                write_offset += height;
                y_pos += 1;
            }
        }

        lightmap.blur_horizontal(area);
        lightmap.blur_vertical(area);

        lightmap.blur_horizontal(area);
        lightmap.blur_vertical(area);

        lightmap.blur_horizontal(area);

        task.area = light_area;
        task.colors = lightmap.colors;
        task.masks = lightmap.masks;
        task.width = lightmap.width;
        task.height = lightmap.height;
        task.finished = true;
        *result = task;

        lightmap.colors = nullptr;
        lightmap.masks = nullptr;
    }, light_area, result);

    m_lightmap_tasks.push_back(UpdateLightMapTask {
        .data = std::move(result),
        .thread = std::move(task)
    });
}

void World::init() {
    m_flames_sprite = sge::TextureAtlasSprite{ Assets::GetTextureAtlas(TextureAsset::Flames0) };
    m_flames_sprite.set_anchor(sge::Anchor::TopLeft);
    m_flames_sprite.set_z(0.4f);
    m_flames_sprite.set_index(0, 0);
    m_flames_sprite.set_color(sge::LinearRgba(150, 150, 150, 7));

    m_cracks_sprite = sge::TextureAtlasSprite{ Assets::GetTextureAtlas(TextureAsset::TileCracks) };
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

    update_lightmap(pos);

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

    update_lightmap(pos);

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

    update_lightmap(pos);

    m_data.changed_tiles.emplace_back(pos, 0);

    reset_tiles(pos, *this);

    this->update_neighbors(pos);
}

void World::update_block(TilePos pos, BlockTypeWithData new_block, uint8_t new_variant) {
    ZoneScoped;

    if (!m_data.is_tilepos_valid(pos)) return;

    const auto index = m_data.get_tile_index(pos);

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

    const auto index = m_data.get_tile_index(pos);

    m_data.blocks[index]->data = new_data;

    m_changed = true;

    reset_tiles(pos, *this);
    update_neighbors(pos);
}

void World::update_block_variant(TilePos pos, uint8_t new_variant) {
    ZoneScoped;

    if (!m_data.is_tilepos_valid(pos)) return;

    const auto index = m_data.get_tile_index(pos);

    m_data.blocks[index]->variant = new_variant;

    m_changed = true;

    reset_tiles(pos, *this);
    update_neighbors(pos);
}

void World::update_block_type(TilePos pos, BlockType new_type) {
    ZoneScoped;

    if (!m_data.is_tilepos_valid(pos)) return;

    const auto index = m_data.get_tile_index(pos);

    m_data.blocks[index]->type = new_type;

    m_changed = true;

    reset_tiles(pos, *this);
    update_neighbors(pos);
}

void World::set_wall(TilePos pos, WallType wall_type) {
    ZoneScoped;

    if (!m_data.is_tilepos_valid(pos)) return;

    const auto index = m_data.get_tile_index(pos);

    m_data.walls[index] = Wall(wall_type);
    m_changed = true;
    m_lightmap_changed = true;

    update_lightmap(pos);

    update_neighbors(pos);

    m_chunk_manager.set_walls_changed(pos);
}

void World::remove_wall(TilePos pos) {
    ZoneScoped;

    if (!m_data.is_tilepos_valid(pos)) return;

    const auto index = m_data.get_tile_index(pos);

    if (m_data.walls[index].has_value()) {
        m_chunk_manager.set_walls_changed(pos);
    }

    m_data.walls[index] = std::nullopt;
    m_changed = true;
    m_lightmap_changed = true;

    m_wall_cracks.erase(pos);

    update_lightmap(pos);

    m_data.changed_tiles.emplace_back(pos, 0);

    this->update_neighbors(pos);
}

void World::update_wall(TilePos pos, WallType new_type, uint8_t new_variant) {
    ZoneScoped;

    if (!m_data.is_tilepos_valid(pos)) return;

    const auto index = m_data.get_tile_index(pos);
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

    for (size_t i = 0; i < m_tile_dig_animations.size(); ++i) {
        TileDigAnimation& anim = m_tile_dig_animations[i];

        if (anim.progress >= 1.0f) {
            m_tile_dig_animations.erase(i);
            continue;
        }

        anim.progress += 7.5f * sge::Time::DeltaSeconds();

        if (anim.progress >= 0.5f) {
            anim.scale = 1.0f - anim.progress;
        } else {
            anim.scale = anim.progress;
        }
    }

    for (uint32_t i = 0; i < m_lightmap_tasks.size(); ++i) {
        auto& task = m_lightmap_tasks[i];
        auto result = task.data->load();
        if (result.finished) {
            using Constants::LIGHTMAP_CHUNK_SIZE;

            glm::uvec2 offset = glm::uvec2(result.area.min);
            const glm::uvec2 size = result.area.size();

            const glm::uvec2 start_chunk_pos = offset / LIGHTMAP_CHUNK_SIZE;

            glm::uvec2 remaining_size = result.area.size();
            glm::uvec2 chunk_pos = start_chunk_pos;
            glm::uvec2 write_offset = glm::uvec2(LIGHTMAP_INSET);

            while (remaining_size.y > 0) {
                const uint32_t write_height = glm::min(offset.y + remaining_size.y, chunk_pos.y * LIGHTMAP_CHUNK_SIZE + LIGHTMAP_CHUNK_SIZE) - offset.y;

                while (remaining_size.x > 0) {
                    const uint32_t write_width = glm::min(offset.x + remaining_size.x, chunk_pos.x * LIGHTMAP_CHUNK_SIZE + LIGHTMAP_CHUNK_SIZE) - offset.x;

                    StaticLightMapChunk* lightmap_chunk = chunk_manager().light_chunks().get_unchecked(chunk_pos);
                    if (lightmap_chunk != nullptr) {
                        const glm::uvec2 texture_offset = offset % LIGHTMAP_CHUNK_SIZE;
                        lightmap_chunk->copy_lightmap_area(result.colors, result.masks, write_offset, result.width, texture_offset, glm::uvec2(write_width, write_height));
                    }

                    offset.x += write_width;
                    remaining_size.x -= write_width;
                    write_offset.x += write_width;
                    chunk_pos.x += 1;
                }

                remaining_size.y -= write_height;
                write_offset.y += write_height;
                remaining_size.x = size.x;
                write_offset.x = uint32_t(LIGHTMAP_INSET);
                offset.x = result.area.min.x;
                offset.y += write_height;
                chunk_pos.x = start_chunk_pos.x;
                chunk_pos.y += 1;
            }

            delete[] result.colors;
            delete[] result.masks;
            
            task.thread.join();
            m_lightmap_tasks.erase(i);
        }
    }
}

void World::fixed_update(const sge::Rect& player_rect, Inventory& inventory) {
    m_dropped_items.update_lookup();

    for (DroppedItem& item : m_dropped_items) {
        item.update(m_data, sge::Time::FixedDeltaSeconds());
    }

    m_dropped_items.for_each_neighbor(player_rect.center(), [&](size_t, DroppedItem& item) {
        if (!item.picked() && item.follow_player(player_rect, inventory)) {
            inventory.add_item_stack(item.item());
            item.set_picked();
        }
    });

    for (size_t i = 0; i < m_dropped_items.size(); ++i) {
        if (m_dropped_items[i].picked()) {
            m_dropped_items.erase(i);
        }
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
            
            if (item_b.stack >= item_max_stack)
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

            dropped_item_b.set_picked();
        });
    }
}

void World::draw(const sge::Camera& camera) {
    ZoneScoped;

    GameRenderer::BeginOrderMode();
        for (const TilePos& tile_pos : m_data.torches) {
            for (const glm::vec2& offset : m_offsets) {
                const glm::vec2 flame_pos = tile_pos.to_world_pos() - glm::vec2((20.0f - 16.0f) / 2.0f, 0.0f);
                m_flames_sprite.set_position(flame_pos + offset);
                GameRenderer::DrawAtlasSpriteWorldPremultiplied(m_flames_sprite);
            }
        }
    GameRenderer::EndOrderMode();

    m_cracks_sprite.set_scale(1.0f);

    for (const auto& [pos, cracks] : m_block_cracks) {
        m_cracks_sprite.set_position(pos.to_world_pos_center());
        m_cracks_sprite.set_index(cracks);
        m_cracks_sprite.set_z(0.4f);

        GameRenderer::DrawAtlasSpriteWorld(m_cracks_sprite);
    }

    for (const auto& [pos, cracks] : m_wall_cracks) {
        m_cracks_sprite.set_position(pos.to_world_pos_center());
        m_cracks_sprite.set_index(cracks);
        m_cracks_sprite.set_z(0.2f);

        GameRenderer::DrawAtlasSpriteWorld(m_cracks_sprite);
    }

    m_cracks_sprite.set_z(1.0f);

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
                m_cracks_sprite.set_position(position);
                m_cracks_sprite.set_scale(scale);
                m_cracks_sprite.set_index(cracks->second);

                GameRenderer::DrawAtlasSpriteWorld(m_cracks_sprite, sge::Order(1));
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
