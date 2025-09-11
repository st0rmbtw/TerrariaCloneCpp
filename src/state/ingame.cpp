#include <SGE/engine.hpp>
#include <SGE/input.hpp>
#include <SGE/utils/text.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>

#include "../app.hpp"
#include "../ui/ui.hpp"
#include "../particles.hpp"
#include "../renderer/renderer.hpp"
#include "../background.hpp"
#include "../diagnostic/frametime.hpp"

#include "ingame.hpp"

static constexpr float INVENTORY_TITLE_SIZE = 22.0f;
static constexpr float MIN_CURSOR_SCALE = 1.2;
static constexpr float MAX_CURSOR_SCALE = MIN_CURSOR_SCALE + 0.1;
static constexpr float INVENTORY_PADDING = 10;
static constexpr float HOTBAR_SLOT_SIZE = 40.0f;
static constexpr float INVENTORY_SLOT_SIZE = HOTBAR_SLOT_SIZE * 1.15;
static constexpr float HOTBAR_SLOT_SIZE_SELECTED = HOTBAR_SLOT_SIZE * 1.3;
static constexpr float INVENTORY_CELL_MARGIN = 4.0f;

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
    update_ui_cursor();

    Inventory& inventory = m_player.inventory();

    if (sge::Input::JustPressed(sge::Key::Escape)) {
        m_show_extra_ui = !m_show_extra_ui;
        inventory.return_taken_item();
    }

    if (sge::Input::JustPressed(sge::Key::F10)) {
        m_ui_show_fps = !m_ui_show_fps;
    }

    // if (m_show_extra_ui) {
    //     if (!sge::Input::IsMouseOverUi() && inventory.has_taken_item() && sge::Input::JustPressed(sge::MouseButton::Right)) {
    //         m_player.throw_item(m_world, TAKEN_ITEM_INDEX);
    //     }
    // }

    auto select_hotbar_slot = [this](Inventory& inventory, uint8_t slot) {
        m_hotbar_slot_anim = 0.0f;
        m_previous_selected_slot = inventory.selected_slot();
        inventory.set_selected_slot(slot);
    };

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

#if 0 
void InGameState::draw_inventory() noexcept {
    ZoneScoped;

    static constexpr float TITLE_OFFSET = 4.0f;
    static constexpr float DEFAULT_TEXT_SIZE = 14.0f;
    static constexpr uint32_t inventory_index = 0;
    static constexpr uint32_t item_index = 1;
    static constexpr uint32_t text_index = item_index + 1;

    const Inventory& inventory = m_player.inventory();

    GameRenderer::BeginOrderMode();

    glm::vec2 offset = glm::vec2(INVENTORY_PADDING, INVENTORY_TITLE_SIZE);
    glm::vec2 item_size = glm::vec2(0.0f);
    glm::vec2 cell_size = glm::vec2(0.0f);

    const ItemSlot& taken_item = inventory.taken_item();
    bool item_is_taken = taken_item.has_item();

    const float hotbar_slot_size_selected = glm::mix(HOTBAR_SLOT_SIZE, HOTBAR_SLOT_SIZE_SELECTED, m_hotbar_slot_anim);
    const float hotbat_slot_size_previously_selected = glm::mix(HOTBAR_SLOT_SIZE_SELECTED, HOTBAR_SLOT_SIZE, m_hotbar_slot_anim);

    const sge::Font& font = Assets::GetFont(FontAsset::AndyBold);

    for (uint8_t i = 0; i < CELLS_IN_ROW; ++i) {
        TextureAsset texture;
        glm::vec2 padding = glm::vec2(0.0f);
        float text_size = DEFAULT_TEXT_SIZE;

        const ItemSlot& item_slot = inventory.get_item(i);
        const bool item_selected = inventory.selected_slot() == i;

        if (item_slot.has_item()) {
            item_size = Assets::GetItemTexture(item_slot.item->id).size();
            item_size = glm::min(item_size, glm::vec2(32.0f)); // The maximum size of an item image is 32px
        }

        if (m_show_extra_ui) {
            texture = TextureAsset::UiInventoryHotbar;
            cell_size = glm::vec2(INVENTORY_SLOT_SIZE);
            item_size *= 0.95f;
            text_size *= 1.15f;
        } else if (item_selected) {
            texture = TextureAsset::UiInventorySelected;
            cell_size = glm::vec2(hotbar_slot_size_selected);
            text_size = glm::mix(text_size, text_size * 1.3f, m_hotbar_slot_anim);
            padding.y = (HOTBAR_SLOT_SIZE_SELECTED - hotbar_slot_size_selected) * 0.5f;
        } else {
            const float size = m_previous_selected_slot == i ? hotbat_slot_size_previously_selected : HOTBAR_SLOT_SIZE;

            texture = TextureAsset::UiInventoryBackground;
            item_size *= 0.9f;
            cell_size = glm::vec2(size);
            text_size = m_previous_selected_slot == i ? glm::mix(text_size * 1.3f, text_size, m_hotbar_slot_anim) : text_size;
            padding.y = (HOTBAR_SLOT_SIZE_SELECTED - cell_size.y) * 0.5f;
        }

        draw_inventory_cell(i, cell_size, offset + padding, texture, sge::Order(inventory_index));

        if (item_slot.has_item()) {
            const glm::vec2 position = offset + padding + cell_size * 0.5f;
            draw_item_with_stack(font, item_size, text_size, position, item_slot.item.value(), sge::Order(item_index), sge::Order(text_index));
        }

        // Draw cell index
        if (item_slot.has_item() || m_show_extra_ui) {
            float index_size = text_size;
            float index_color = 0.9f;
            if (m_show_extra_ui && item_selected && !item_is_taken) {
                index_size *= 1.15f;
                index_color = 1.0f;
            }

            const float a = text_size / 14.0f;
            const glm::vec2 position = offset + padding + glm::vec2(6.0f, 6.0f) * a;

            const char index = '0' + (i + 1) % 10;
            GameRenderer::DrawCharUI(index, position, index_size, sge::LinearRgba(index_color), font, sge::Order(text_index));
        }

        offset.x += cell_size.x + INVENTORY_CELL_MARGIN;
    }

    if (m_show_extra_ui) {
        const sge::RichText text = rich_text("Inventory", INVENTORY_TITLE_SIZE, sge::LinearRgba(0.8f));
        const glm::vec2 position = glm::vec2(INVENTORY_PADDING + INVENTORY_SLOT_SIZE * 0.5f, TITLE_OFFSET);
        GameRenderer::DrawTextUI(text, position, font, sge::Order(text_index));
    } else {
        const ItemSlot& item_slot = inventory.get_item(inventory.selected_slot());
        const std::string_view& title = item_slot.has_item() ? item_slot.item->name : "Items";
        const sge::RichText text = sge::rich_text(title, INVENTORY_TITLE_SIZE, sge::LinearRgba(0.8f));

        const glm::vec2 bounds = sge::calculate_text_bounds(font, text);

        const glm::vec2 position = glm::vec2((offset.x - bounds.x) * 0.5f, TITLE_OFFSET);
        GameRenderer::DrawTextUI(text, position, font, sge::Order(text_index));
    }

    if (m_show_extra_ui) {
        offset.y += INVENTORY_SLOT_SIZE + INVENTORY_CELL_MARGIN;

        cell_size = glm::vec2(INVENTORY_SLOT_SIZE);

        for (uint8_t y = 1; y < INVENTORY_ROWS; ++y) {
            offset.x = INVENTORY_PADDING;

            for (uint8_t x = 0; x < CELLS_IN_ROW; ++x) {
                const uint8_t index = y * CELLS_IN_ROW + x;

                const ItemSlot item_slot = inventory.get_item(index);
                if (item_slot.has_item()) {
                    item_size = Assets::GetItemTexture(item_slot.item->id).size();
                    item_size = glm::min(item_size, glm::vec2(32.0f)); // The maximum size of an item image is 32px
                    item_size *= 0.95f;
                }

                draw_inventory_cell(index, cell_size, offset, TextureAsset::UiInventoryBackground, sge::Order(inventory_index));

                if (item_slot.has_item()) {
                    const glm::vec2 position = offset + cell_size * 0.5f;
                    draw_item_with_stack(font, item_size, 14.0f * 1.15f, position, item_slot.item.value(), sge::Order(item_index), sge::Order(text_index));
                }

                offset.x += cell_size.x + INVENTORY_CELL_MARGIN;
            }

            offset.y += cell_size.y + INVENTORY_CELL_MARGIN;
        }
    }

    // Draw pangramas in different languages to test text rendering
    // const sge::RichText text = rich_text(
    //     sge::RichTextSection("The quick brown fox jumps over the lazy dog æ\n", 48.0f, sge::LinearRgba(0.9f, 0.4f, 0.4f)),
    //     sge::RichTextSection("Съешь ещё этих мягких французских булок, да выпей же чаю\n", 48.0f, sge::LinearRgba(0.4f, 0.9f, 0.4f))
    // );
    // const glm::vec2 bounds = sge::calculate_text_bounds(font, text);
    // GameRenderer::DrawTextUI(text, window_size * 0.5f - bounds * 0.5f, font, sge::Order(inventory_index));

    GameRenderer::EndOrderMode();
}

#else

void InGameState::draw_inventory() noexcept {
    GameRenderer::BeginOrderMode();

    Inventory& inventory = m_player.inventory();

    UI::BeginLayout(LayoutDesc(LayoutOrientation::Vertical).with_gap(4.0f).with_padding(UiRect(10.0f, 0.0f, 10.0f, 0.0f)));
    {
        ElementState element;

        const uint32_t rows_count = m_show_extra_ui ? INVENTORY_ROWS : 1;

        const glm::vec2 hotbar_selected_size = glm::mix(glm::vec2(HOTBAR_SLOT_SIZE), glm::vec2(HOTBAR_SLOT_SIZE_SELECTED), m_hotbar_slot_anim);
        const glm::vec2 hotbar_unselected_size = glm::mix(glm::vec2(HOTBAR_SLOT_SIZE_SELECTED), glm::vec2(HOTBAR_SLOT_SIZE), m_hotbar_slot_anim);

        UI::BeginLayout(LayoutDesc(LayoutOrientation::Horizontal).with_gap(4.0f));
        {
            for (uint32_t i = 0; i < CELLS_IN_ROW; ++i) {
                const bool selected = inventory.selected_slot() == i;

                TextureAsset texture = TextureAsset::UiInventoryBackground;
                glm::vec2 size = glm::vec2(0.0f);

                if (m_show_extra_ui) {
                    size = glm::vec2(INVENTORY_SLOT_SIZE);
                    texture = TextureAsset::UiInventoryBackground;
                } else if (selected) {
                    size = hotbar_selected_size;
                    texture = TextureAsset::UiInventorySelected;
                } else {
                    texture = TextureAsset::UiInventoryHotbar;
                    size = hotbar_unselected_size;
                }

                element = UI::Element(size);
                {
                    sge::Sprite cell_sprite(Assets::GetTexture(texture));
                    cell_sprite.set_position(element.position);
                    cell_sprite.set_anchor(sge::Anchor::TopLeft);
                    cell_sprite.set_custom_size(size);
                    cell_sprite.set_color(sge::LinearRgba(1.0f, 1.0f, 1.0f, 0.8f));
                    GameRenderer::DrawSpriteUI(cell_sprite);
                }
            }
        }
        UI::EndLayout();

        if (m_show_extra_ui) {
            const glm::vec2 size = glm::vec2(INVENTORY_SLOT_SIZE);

            for (uint32_t j = 1; j < rows_count; ++j) {
                UI::BeginLayout(LayoutDesc(LayoutOrientation::Horizontal).with_gap(4.0f));
                {
                    for (uint32_t i = 0; i < CELLS_IN_ROW; ++i) {
                        element = UI::Element(size);
                        {
                            sge::Sprite cell_sprite(Assets::GetTexture(TextureAsset::UiInventoryBackground));
                            cell_sprite.set_position(element.position);
                            cell_sprite.set_anchor(sge::Anchor::TopLeft);
                            cell_sprite.set_custom_size(size);
                            cell_sprite.set_color(sge::LinearRgba(1.0f, 1.0f, 1.0f, 0.8f));
                            GameRenderer::DrawSpriteUI(cell_sprite);
                        }
                    }
                }
                UI::EndLayout(); 
            }
        }

    }
    UI::EndLayout();

    GameRenderer::EndOrderMode();
}

#endif

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

void InGameState::draw_inventory_cell(uint8_t index, const glm::vec2& size, const glm::vec2& position, TextureAsset texture, sge::Order order) {
    sge::Sprite cell_sprite(Assets::GetTexture(texture));
    cell_sprite.set_position(position);
    cell_sprite.set_anchor(sge::Anchor::TopLeft);
    cell_sprite.set_custom_size(size);
    cell_sprite.set_color(sge::LinearRgba(1.0f, 1.0f, 1.0f, 0.8f));
    GameRenderer::DrawSpriteUI(cell_sprite, order);
}

void InGameState::draw_ui() noexcept {
    draw_inventory();

    const Inventory& inventory = m_player.inventory();
    const sge::Font& font = Assets::GetFont(FontAsset::AndyBold);

    if (m_ui_show_fps) {
        const sge::RichText text = sge::rich_text(m_ui_fps_text, 22.0f, sge::LinearRgba(0.8f));
        GameRenderer::DrawTextUI(text, glm::vec2(10.0f, m_camera.viewport().y - 10.0f - 22.0f), font, sge::Order(0, false));
    }

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

BaseState* InGameState::GetNextState() {
    return nullptr;
}
