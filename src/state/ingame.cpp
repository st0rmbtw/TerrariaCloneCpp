#include <SGE/engine.hpp>
#include <SGE/input.hpp>
#include <SGE/utils/text.hpp>
#include <SGE/types/color.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>

#include "../app.hpp"
#include "../ui/ui.hpp"
#include "../particles.hpp"
#include "../renderer/renderer.hpp"
#include "../background.hpp"
#include "../diagnostic/frametime.hpp"
#include "SGE/types/rich_text.hpp"

#include "ingame.hpp"

static constexpr float UI_PADDING = 10.0f;

static constexpr float INVENTORY_TITLE_SIZE = 22.0f;
static constexpr float MIN_CURSOR_SCALE = 1.2f;
static constexpr float MAX_CURSOR_SCALE = MIN_CURSOR_SCALE + 0.1f;
static constexpr float HOTBAR_SLOT_SIZE = 40.0f;
static constexpr float INVENTORY_SLOT_SIZE = HOTBAR_SLOT_SIZE * 1.15f;
static constexpr float HOTBAR_SLOT_SIZE_SELECTED = HOTBAR_SLOT_SIZE * 1.3f;
static constexpr float INVENTORY_GAP = 4.0f;

namespace UiTypeID {
    enum : uint8_t {
        HotbarSlot = 0,
        InventorySlot,
        InventorySlotItem,
        InventorySlotIndex,
        Text
    };
}

struct UiHotbarSlotData {
    TextureAsset texture;
};

struct UiInventorySlotItemData {
    ItemId item_id;
};

struct UiInventorySlotIndexData {
    sge::LinearRgba color;
    float size;
    char index;
    FontAsset font;
};

InGameState::InGameState() :
    m_camera{ sge::CameraOrigin::Center, sge::CoordinateSystem {
        .up = sge::CoordinateDirectionY::Negative,
        .forward = sge::CoordinateDirectionZ::Negative,
    }}
{
    m_fps_update_timer = sge::Timer::from_seconds(0.5f, sge::TimerMode::Repeating);
    m_fps_update_timer.set_finished();

    m_camera.set_viewport(App::GetWindowResolution());
    m_camera.set_zoom(1.0f);

    m_world.init();
    m_world.generate(200, 500, 0);

    GameRenderer::InitWorldRenderer(m_world.data());
    Background::SetupWorldBackground(m_world);

    m_player.init();
    m_player.set_position(m_world, glm::vec2(m_world.spawn_point()) * Constants::TILE_SIZE);
    m_camera.set_position(m_player.draw_position());

    Inventory& inventory = m_player.inventory();
    inventory.add_item_stack(ITEM_COPPER_AXE);
    inventory.add_item_stack(ITEM_COPPER_PICKAXE);
    inventory.add_item_stack(ITEM_COPPER_HAMMER);
    inventory.add_item_stack(ITEM_DIRT_BLOCK.with_stack(1000));
    inventory.add_item_stack(ITEM_DIRT_BLOCK.with_stack(2500));
    inventory.add_item_stack(ITEM_STONE_BLOCK.with_max_stack());
    inventory.add_item_stack(ITEM_WOOD_BLOCK.with_max_stack());
    inventory.add_item_stack(ITEM_TORCH.with_max_stack());
    inventory.add_item_stack(ITEM_WOOD_WALL.with_max_stack());
    inventory.add_item_stack(ITEM_DIRT_BLOCK.with_max_stack());

    m_fps_update_timer = sge::Timer::from_seconds(0.5f, sge::TimerMode::Repeating);
    m_fps_update_timer.set_finished();

    m_cursor_foreground_color = sge::LinearRgba(1.0, 0.08, 0.58);
    m_cursor_background_color = sge::LinearRgba(0.9, 0.9, 0.9);

    m_cursor_background
        .set_texture(Assets::GetTexture(TextureAsset::UiCursorBackground))
        .set_color(m_cursor_background_color)
        .set_anchor(sge::Anchor::TopLeft)
        .set_outline_color(m_cursor_background_color)
        .set_outline_thickness(0.03);

    m_cursor_foreground
        .set_texture(Assets::GetTexture(TextureAsset::UiCursorForeground))
        .set_color(m_cursor_foreground_color)
        .set_anchor(sge::Anchor::TopLeft);
}

InGameState::~InGameState() {
    m_world.data().lightmap_tasks_wait();
    m_world.chunk_manager().destroy();
}

glm::vec2 InGameState::camera_follow_player() noexcept {
    static constexpr float OFFSET = Constants::WORLD_BOUNDARY_OFFSET;

    glm::vec2 position = m_player.draw_position();

    const sge::Rect area = m_world.playable_area() * Constants::TILE_SIZE;
    const sge::Rect& camera_area = m_camera.get_projection_area();

    const float left = camera_area.min.x;
    const float right = camera_area.max.x;

    const float top = camera_area.min.y;
    const float bottom = camera_area.max.y;

    if (position.x + left < area.min.x + OFFSET) position.x = area.min.x - left + OFFSET;
    if (position.x + right > area.max.x - OFFSET) position.x = area.max.x - right - OFFSET;

    if (position.y + top < area.min.y) position.y = area.min.y - top;
    if (position.y + bottom > area.max.y) position.y = area.max.y - bottom;

    return position;
}

#if DEBUG_TOOLS
glm::vec2 InGameState::camera_free() noexcept {
    const float dt = sge::Time::DeltaSeconds();
    glm::vec2 position = m_camera.position();

    float speed = 2000.0f;

    if (sge::Input::Pressed(sge::Key::LeftShift)) speed *= 2.0f;
    if (sge::Input::Pressed(sge::Key::LeftAlt)) speed /= 5.0f;

    if (sge::Input::Pressed(sge::Key::A)) position.x -= speed * dt;
    if (sge::Input::Pressed(sge::Key::D)) position.x += speed * dt;
    if (sge::Input::Pressed(sge::Key::W)) position.y -= speed * dt;
    if (sge::Input::Pressed(sge::Key::S)) position.y += speed * dt;

    const sge::Rect& camera_area = m_camera.get_projection_area();

    if (position.y + camera_area.min.y < 0.0f) position.y = 0.0f - camera_area.min.y;

    return position;
}
#endif

void InGameState::PreUpdate() {
    ZoneScoped;

#if DEBUG
    if (sge::Input::JustPressed(sge::Key::B)) {
        SGE_DEBUG_BREAK();
    }
#endif

    ParticleManager::DeleteExpired();

    m_player.pre_update();
    m_world.clear_lights();

#if DEBUG_TOOLS
    if (sge::Input::JustPressed(sge::Key::F)) m_free_camera = !m_free_camera;
#endif
}

void InGameState::Update() {
    ZoneScoped;

    float scale_speed = 2.f;

    if (sge::Input::Pressed(sge::Key::LeftShift)) scale_speed *= 4.f;
    if (sge::Input::Pressed(sge::Key::LeftAlt)) scale_speed /= 4.f;

    if (sge::Input::Pressed(sge::Key::Minus)) {
        const float zoom = m_camera.zoom() + scale_speed * sge::Time::DeltaSeconds();
        m_camera.set_zoom(glm::clamp(zoom, Constants::CAMERA_MAX_ZOOM, Constants::CAMERA_MIN_ZOOM));
    }

    if (sge::Input::Pressed(sge::Key::Equals)) {
        const float zoom = m_camera.zoom() - scale_speed * sge::Time::DeltaSeconds();
        m_camera.set_zoom(glm::clamp(zoom, Constants::CAMERA_MAX_ZOOM, Constants::CAMERA_MIN_ZOOM));
    }

#if DEBUG_TOOLS
    if (m_free_camera && sge::Input::Pressed(sge::MouseButton::Right)) {
        m_player.set_position(m_world, m_camera.screen_to_world(sge::Input::MouseScreenPosition()));
    }
    const glm::vec2 position = m_free_camera ? camera_free() : camera_follow_player();
#else
    const glm::vec2 position = camera_follow_player();
#endif

    m_camera.set_position(position);

    m_camera.update();
    m_world.update(m_camera);

    Background::UpdateInGame(m_camera, m_world);

    m_player.update(m_world);

    update_ui();

    m_world.add_light(Light {
        .color = glm::vec3(1.0f, 0.0f, 0.0f),
        .pos = get_lightmap_pos(m_camera.screen_to_world(sge::Input::MouseScreenPosition())),
        .size = glm::uvec2(2)
    });

    if (sge::Input::JustPressed(sge::Key::Q)) {
        m_lights.push_back(Light {
            .color = glm::linearRand(glm::vec3(0.0f), glm::vec3(1.0f)),
            .pos = get_lightmap_pos(m_camera.screen_to_world(sge::Input::MouseScreenPosition())),
            .size = glm::uvec2(2)
        });
    }

    for (const Light& light : m_lights) {
        m_world.add_light(light);
    }

#if DEBUG_TOOLS
    if (sge::Input::Pressed(sge::Key::K)) {
        const glm::vec2 position = m_camera.screen_to_world(sge::Input::MouseScreenPosition());

        for (int i = 0; i < 500; ++i) {
            const glm::vec2 velocity = glm::diskRand(1.5f);

            ParticleManager::SpawnParticle(
                ParticleBuilder::create(Particle::Type::Grass, position, velocity, 5.0f)
                    .with_rotation_speed(glm::pi<float>() / 12.0f)
                    .with_light(glm::vec3(0.1f, 0.9f, 0.1f))
            );
        }
    }
#endif
}

void InGameState::FixedUpdate() {
    ZoneScoped;

#if DEBUG_TOOLS
    const bool handle_input = !m_free_camera;
#else
    constexpr bool handle_input = true;
#endif

    m_player.fixed_update(m_camera, m_world, handle_input);
    m_world.fixed_update(m_player.rect(), m_player.inventory());

    GameRenderer::UpdateLight();

    ParticleManager::Update(m_world);
}

void InGameState::Render() {
    ZoneScoped;

    GameRenderer::Begin(m_camera, m_world);

    Background::Draw();

    m_world.draw(m_camera);
    m_player.draw();

    ParticleManager::Draw();

    draw_ui();

    GameRenderer::Render(m_camera, m_world);
}

void InGameState::PostRender() {
    ZoneScoped;
    
    if (m_world.chunk_manager().any_chunks_to_destroy()) {
        sge::Engine::Renderer().CommandQueue()->WaitIdle();
        m_world.chunk_manager().destroy_hidden_chunks();
    }

#if DEBUG
    if (sge::Input::Pressed(sge::Key::C)) {
        sge::Engine::Renderer().PrintDebugInfo();
    }
#endif
}

void InGameState::update_ui_cursor() noexcept {
    if (m_cursor_anim_progress >= 1.0f) {
        m_cursor_anim_dir = AnimationDirection::Backward;
    } else if (m_cursor_anim_progress <= 0.0f) {
        m_cursor_anim_dir = AnimationDirection::Forward;
    }

    switch (m_cursor_anim_dir) {
    case AnimationDirection::Backward:
        m_cursor_anim_progress -= 2.0f * sge::Time::DeltaSeconds();
        break;
    case AnimationDirection::Forward:
        m_cursor_anim_progress += 2.0f * sge::Time::DeltaSeconds();
        break;
    }

    m_cursor_anim_progress = glm::clamp(m_cursor_anim_progress, 0.0f, 1.0f);

    const float scale = MIN_CURSOR_SCALE + m_cursor_anim_progress * (MAX_CURSOR_SCALE - MIN_CURSOR_SCALE);
    m_cursor_scale = scale;

    m_cursor_background.set_position(sge::Input::MouseScreenPosition());
    m_cursor_foreground.set_position(sge::Input::MouseScreenPosition() + glm::vec2(3.0f));

    m_cursor_background.set_scale(glm::vec2(scale));
    m_cursor_foreground.set_scale(glm::vec2(scale));

    m_cursor_foreground.set_color(m_cursor_foreground_color * (0.7f + 0.3f * m_cursor_anim_progress));
}

void InGameState::update_ui() noexcept {
    UI::Update();

    update_ui_cursor();

    Inventory& inventory = m_player.inventory();

    if (sge::Input::JustPressed(sge::Key::Escape)) {
        m_show_extra_ui = !m_show_extra_ui;
        inventory.return_taken_item();
    }

    if (sge::Input::JustPressed(sge::Key::F10)) {
        m_ui_show_fps = !m_ui_show_fps;
    }

    if (m_show_extra_ui) {
        if (!UI::IsMouseOverUi() && inventory.has_taken_item() && sge::Input::JustPressed(sge::MouseButton::Right)) {
            m_player.throw_item(m_world, TAKEN_ITEM_INDEX);
        }
    }

    if (sge::Input::JustPressed(sge::Key::Digit1)) select_hotbar_slot(inventory, 0);
    if (sge::Input::JustPressed(sge::Key::Digit2)) select_hotbar_slot(inventory, 1);
    if (sge::Input::JustPressed(sge::Key::Digit3)) select_hotbar_slot(inventory, 2);
    if (sge::Input::JustPressed(sge::Key::Digit4)) select_hotbar_slot(inventory, 3);
    if (sge::Input::JustPressed(sge::Key::Digit5)) select_hotbar_slot(inventory, 4);
    if (sge::Input::JustPressed(sge::Key::Digit6)) select_hotbar_slot(inventory, 5);
    if (sge::Input::JustPressed(sge::Key::Digit7)) select_hotbar_slot(inventory, 6);
    if (sge::Input::JustPressed(sge::Key::Digit8)) select_hotbar_slot(inventory, 7);
    if (sge::Input::JustPressed(sge::Key::Digit9)) select_hotbar_slot(inventory, 8);
    if (sge::Input::JustPressed(sge::Key::Digit0)) select_hotbar_slot(inventory, 9);

    for (const float scroll : sge::Input::ScrollEvents()) {
        const int next_index = static_cast<int>(inventory.selected_slot()) - static_cast<int>(glm::sign(scroll));
        const int new_index = (next_index % CELLS_IN_ROW + CELLS_IN_ROW) % CELLS_IN_ROW;
        select_hotbar_slot(inventory, static_cast<uint8_t>(new_index));
    }

    if (m_ui_show_fps) {
        if (m_fps_update_timer.tick(sge::Time::Delta()).just_finished()) {
            const int fps = FrameTime::GetAverageFPS();
            m_ui_fps_text = std::to_string(fps);
        }
    }

    if (m_hotbar_slot_anim < 1.0f) {
        m_hotbar_slot_anim += sge::Time::DeltaSeconds() * 8.0f;
        if (m_hotbar_slot_anim > 1.0f) m_hotbar_slot_anim = 1.0f;
    }
}

void InGameState::draw_cursor() noexcept {
    const sge::Font& font = Assets::GetFont(FontAsset::AndyBold);
    Inventory& inventory = m_player.inventory();

    GameRenderer::DrawSpriteUI(m_cursor_background);
    GameRenderer::DrawSpriteUI(m_cursor_foreground);

    const ItemSlot& taken_item = inventory.taken_item();
    const ItemSlot& selected_item = inventory.get_selected_item();
    const glm::vec2 position = m_cursor_background.position() + m_cursor_background.size();

    if (m_show_extra_ui && taken_item.has_item()) {
        const sge::Texture& texture = Assets::GetItemTexture(taken_item.item->id);
        const glm::vec2 size = glm::vec2(texture.size()) * m_cursor_scale;

        draw_item_with_stack(font, size, 16.0f * m_cursor_scale, position, taken_item.item.value());
    } else if (m_player.can_use_item() && selected_item.has_item() && !UI::IsMouseOverUi()) {
        const sge::Texture& texture = Assets::GetItemTexture(selected_item.item->id);
        const glm::vec2 size = glm::vec2(texture.size()) * m_cursor_scale;

        draw_item(size, position, *selected_item.item);
    }
}

void InGameState::draw_inventory() noexcept {
    UI::Container({
        .padding = UiRect(0.0f, 0.0f, 4.0f, 0.0f),
        .orientation = LayoutOrientation::Vertical
    }, [this] {
        Inventory& inventory = m_player.inventory();
        const sge::Font& font = Assets::GetFont(FontAsset::AndyBold);

        const ItemSlot selected_item = inventory.get_item(inventory.selected_slot());
        
        std::string_view title = "Items";
        if (m_show_extra_ui) {
            title = "Inventory";
        } else if (selected_item.has_item()) {
            title = selected_item.item->name;
        }
        
        UI::Text<UiTypeID::Text>(
            font,
            sge::rich_text(title, INVENTORY_TITLE_SIZE, sge::LinearRgba(0.8f)),
            {
                .self_alignment = m_show_extra_ui ? Alignment::Start : Alignment::Center
            }
        );

        UI::Container({
            .gap = INVENTORY_GAP,
            .orientation = LayoutOrientation::Vertical,
        }, [&] {
            const uint32_t rows_count = m_show_extra_ui ? INVENTORY_ROWS : 1;

            const glm::vec2 hotbar_selected_size = glm::mix(glm::vec2(HOTBAR_SLOT_SIZE), glm::vec2(HOTBAR_SLOT_SIZE_SELECTED), m_hotbar_slot_anim);
            const glm::vec2 hotbar_unselected_size = glm::mix(glm::vec2(HOTBAR_SLOT_SIZE_SELECTED), glm::vec2(HOTBAR_SLOT_SIZE), m_hotbar_slot_anim);

            for (uint32_t j = 0; j < rows_count; ++j) {
                UI::Container({
                    .size = UiSize(Sizing::Fit(), m_show_extra_ui ? Sizing::Fit() : Sizing::Fixed(HOTBAR_SLOT_SIZE_SELECTED)),
                    .gap = INVENTORY_GAP,
                    .orientation = LayoutOrientation::Horizontal,
                    .vertical_alignment = Alignment::Center,
                }, [&] () {
                    const bool item_is_taken = inventory.taken_item().has_item();

                    TextureAsset texture = TextureAsset::UiInventoryBackground;
                    glm::vec2 back_size = glm::vec2(0.0f);
                    glm::vec2 item_size = glm::vec2(0.0f);
                    
                    for (uint32_t i = 0; i < CELLS_IN_ROW; ++i) {
                        const uint32_t index = j * CELLS_IN_ROW + i;
                        const std::optional<Item>& item = inventory.get_item(index).item;
                        const bool selected = inventory.selected_slot() == index;
                        
                        if (item.has_value()) {
                            item_size = Assets::GetItemTexture(item->id).size();
                            item_size = glm::min(item_size, glm::vec2(32.0f));
                        }
                        
                        float text_size = 14.0f;
                        if (m_show_extra_ui) {
                            texture = TextureAsset::UiInventoryBackground;
                            back_size = glm::vec2(INVENTORY_SLOT_SIZE);
                            item_size *= 0.95f;
                            text_size *= 1.15f;
                        } else if (selected) {
                            texture = TextureAsset::UiInventorySelected;
                            back_size = hotbar_selected_size;
                            text_size = glm::mix(text_size, text_size * 1.3f, m_hotbar_slot_anim);
                        } else {
                            texture = TextureAsset::UiInventoryHotbar;
                            item_size *= 0.9f;
                            back_size = m_previous_selected_slot == i ? hotbar_unselected_size : glm::vec2(HOTBAR_SLOT_SIZE);
                            text_size = m_previous_selected_slot == i ? glm::mix(text_size * 1.3f, text_size, m_hotbar_slot_anim) : text_size;
                        }

                        UI::Element<UiTypeID::HotbarSlot>({
                            .size = UiSize::Fixed(back_size),
                            .horizontal_alignment = Alignment::Center,
                            .vertical_alignment = Alignment::Center,
                        }, [&] {
                            UI::SetCustomData(UiHotbarSlotData {
                                .texture = texture
                            });

                            if (item.has_value()) {
                                UI::AddElement<UiTypeID::InventorySlotItem>(
                                    {
                                        .size = UiSize::Fixed(item_size)
                                    },
                                    UiInventorySlotItemData {
                                        .item_id = item->id
                                    }
                                );
                            }

                            const glm::vec2 padding = glm::vec2(5.0f + (back_size - HOTBAR_SLOT_SIZE) * 0.25f);

                            UI::Container({
                                .size = UiSize::Fill(),
                                .padding = UiRect::Horizontal(padding.x).top(padding.y).bottom(padding.y * 0.5f),
                                .orientation = LayoutOrientation::Vertical,
                            }, [&] {
                                if (index < CELLS_IN_ROW && (item.has_value() || m_show_extra_ui)) {
                                    float index_size = text_size;
                                    float index_color = 0.9f;
                                    if (m_show_extra_ui && selected && !item_is_taken) {
                                        index_size *= 1.15f;
                                        index_color = 1.0f;
                                    }
                                    const char index = '0' + (i + 1) % 10;

                                    const glm::vec2 size = sge::calculate_text_bounds(font, 1, &index, index_size);
                                    UI::AddElement<UiTypeID::InventorySlotIndex>(
                                        {
                                            .size = UiSize::Fixed(size),
                                            .self_alignment = Alignment::End
                                        },
                                        UiInventorySlotIndexData {
                                            .color = sge::LinearRgba(index_color),
                                            .size = index_size,
                                            .index = index,
                                            .font = FontAsset::AndyBold,
                                        }
                                    );
                                }

                                if (item.has_value() && item->stack > 1) {
                                    sge::LinearRgba color = sge::LinearRgba(0.9f);
                                    UI::Spacer();

                                    UI::Text<UiTypeID::Text>(
                                        font,
                                        sge::rich_text(temp_format("{}", item->stack), text_size, color),
                                        {
                                            .self_alignment = Alignment::Center
                                        }
                                    );
                                }
                            });

                            if (index < CELLS_IN_ROW && !m_show_extra_ui) {
                                UI::OnClick([this, &inventory, index](sge::MouseButton button) {
                                    if (button == sge::MouseButton::Left) {
                                        select_hotbar_slot(inventory, index);
                                    }
                                });
                            } else {
                                UI::OnClick([&inventory, index](sge::MouseButton button) {
                                    if (button == sge::MouseButton::Left) {
                                        if (inventory.has_taken_item()) {
                                            inventory.put_item(index);
                                        } else {
                                            inventory.take_item(index);
                                        }
                                    } else if (button == sge::MouseButton::Right) {
                                        inventory.take_item(index, 1);
                                    }
                                });
                            }
                        });
                    }
                });
            }
        });
    });
}

void InGameState::draw_item(const glm::vec2& item_size, const glm::vec2& position, const Item& item, sge::Order item_order) {
    sge::Sprite item_sprite(Assets::GetItemTexture(item.id));
    item_sprite.set_position(position);
    item_sprite.set_anchor(sge::Anchor::Center);
    item_sprite.set_custom_size(item_size);
    item_sprite.set_color(sge::LinearRgba::white());
    GameRenderer::DrawSpriteUI(item_sprite, item_order);
}

void InGameState::draw_item_with_stack(const sge::Font& font, const glm::vec2& item_size, float stack_size, const glm::vec2& position, const Item& item, sge::Order item_order, sge::Order stack_order) {
    draw_item(item_size, position, item, item_order);

    if (item.stack > 1) {
        const std::string stack_string = std::to_string(item.stack);
        const sge::RichText text = sge::rich_text(stack_string, stack_size, sge::LinearRgba(0.9f));
        const float a = stack_size / 14.0f;
        const glm::vec2 stack_position = glm::vec2(position.x - 15.0f * a, position.y + 2.5f * a);
        GameRenderer::DrawTextUI(text, stack_position, font, stack_order);
    }
}

void InGameState::draw_ui() noexcept {
    UI::Start(RootDesc(m_camera.viewport())
        .with_padding(UiRect::Horizontal(UI_PADDING))
    );

    UI::Container({
        .id = ID::Local("LeftSide"),
        .size = UiSize::Height(Sizing::Fill()),
        .orientation = LayoutOrientation::Vertical,
    }, [this] {
        draw_inventory();

        UI::Spacer();

        if (m_ui_show_fps) {
            UI::Text<UiTypeID::Text>(
                Assets::GetFont(FontAsset::AndyBold),
                sge::rich_text(m_ui_fps_text, 22.0f, sge::LinearRgba(0.8f))
            );
            UI::Spacer(UiSize::Height(Sizing::Fixed(UI_PADDING)));
        }
    });

    const std::vector<UiElement>& elements = UI::Finish();

    GameRenderer::BeginOrderMode();

    sge::Sprite sprite(Assets::GetTexture(TextureAsset::Stub));
    for (const UiElement& element : elements) {
        const sge::Order order = sge::Order(element.z_index);

        switch (element.type_id) {
            case UiTypeID::HotbarSlot: {
                const UiHotbarSlotData* data = static_cast<const UiHotbarSlotData*>(element.custom_data);

                sprite.set_texture(Assets::GetTexture(data->texture));
                sprite.set_position(element.position);
                sprite.set_anchor(sge::Anchor::TopLeft);
                sprite.set_custom_size(element.size);
                sprite.set_color(sge::LinearRgba(1.0f, 1.0f, 1.0f, 0.8f));
                GameRenderer::DrawSpriteUI(sprite, order);
            } break;

            case UiTypeID::InventorySlot: {
                sprite.set_texture(Assets::GetTexture(TextureAsset::UiInventoryBackground));
                sprite.set_position(element.position);
                sprite.set_anchor(sge::Anchor::TopLeft);
                sprite.set_custom_size(element.size);
                sprite.set_color(sge::LinearRgba(1.0f, 1.0f, 1.0f, 0.8f));
                GameRenderer::DrawSpriteUI(sprite, order);
            } break;

            case UiTypeID::InventorySlotItem: {
                const UiInventorySlotItemData* data = static_cast<const UiInventorySlotItemData*>(element.custom_data);

                sprite.set_texture(Assets::GetItemTexture(data->item_id));
                sprite.set_position(element.position);
                sprite.set_anchor(sge::Anchor::TopLeft);
                sprite.set_custom_size(element.size);
                sprite.set_color(sge::LinearRgba(1.0f, 1.0f, 1.0f, 0.8f));
                GameRenderer::DrawSpriteUI(sprite, order);
            } break;

            case UiTypeID::InventorySlotIndex: {
                const UiInventorySlotIndexData* data = static_cast<const UiInventorySlotIndexData*>(element.custom_data);
                const sge::Font& font = Assets::GetFont(data->font);
                GameRenderer::DrawCharUI(data->index, element.position, data->size, data->color, font, order);
            } break;

            case UiTypeID::Text: {
                const TextData* data = static_cast<const TextData*>(element.text_data);
                GameRenderer::DrawTextUI(data->sections, data->sections_count, element.position, data->font, order);
            } break;
        }
    }

    GameRenderer::EndOrderMode();

    draw_cursor();
}

BaseState* InGameState::GetNextState() {
    return nullptr;
}
