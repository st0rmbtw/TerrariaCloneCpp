#include "ui.hpp"

#include <set>

#include <tracy/Tracy.hpp>

#include <LLGL/Container/DynamicArray.h>

#include <SGE/time/time.hpp>
#include <SGE/time/timer.hpp>
#include <SGE/types/anchor.hpp>
#include <SGE/types/rich_text.hpp>
#include <SGE/types/color.hpp>
#include <SGE/types/sprite.hpp>
#include <SGE/utils/text.hpp>
#include <SGE/input.hpp>

#include "../renderer/renderer.hpp"
#include "../assets.hpp"

static constexpr float INVENTORY_TITLE_SIZE = 22.0f;
static constexpr float MIN_CURSOR_SCALE = 1.2;
static constexpr float MAX_CURSOR_SCALE = MIN_CURSOR_SCALE + 0.1;
static constexpr float INVENTORY_PADDING = 10;
static constexpr float HOTBAR_SLOT_SIZE = 40;
static constexpr float INVENTORY_SLOT_SIZE = HOTBAR_SLOT_SIZE * 1.15;
static constexpr float HOTBAR_SLOT_SIZE_SELECTED = HOTBAR_SLOT_SIZE * 1.3;
static constexpr float INVENTORY_CELL_MARGIN = 4;

static constexpr uint16_t FRAMETIME_RECORD_MAX_COUNT = 120;

enum class AnimationDirection: uint8_t {
    Backward = 0,
    Forward = 1,
};

enum class UiElement : uint8_t {
    HotbarCell,
    InventoryCell
};

class Element {
public:
    enum : uint8_t {
        None = 0,
        Hovered = 1 << 0,
        Pressed = 1 << 1
    };

    Element(UiElement element_type, uint32_t depth, int data, const sge::Rect& rect) :
        m_element_type(element_type),
        m_data(data),
        m_depth(depth),
        m_rect(rect) {}

    inline void press() noexcept { m_state = m_state | Pressed; }
    inline void hover() noexcept { m_state = m_state | Hovered; }

    [[nodiscard]] inline bool none() const noexcept { return m_state == None; }
    [[nodiscard]] inline bool hovered() const noexcept { return m_state & Hovered; }
    [[nodiscard]] inline bool pressed() const noexcept { return m_state & Pressed; }

    [[nodiscard]] inline const sge::Rect& rect() const noexcept { return m_rect; }
    [[nodiscard]] inline UiElement type() const noexcept { return m_element_type; }
    [[nodiscard]] inline int data() const noexcept { return m_data; }
    [[nodiscard]] constexpr inline int depth() const noexcept { return m_depth; }
private:
    uint8_t m_state = 0;
    UiElement m_element_type;
    int m_data = 0;
    uint32_t m_depth = 0;
    sge::Rect m_rect;
};

struct ElementSort {
    constexpr bool operator()(const Element& a, const Element& b) const {
        return a.depth() > b.depth();
    }
};

static struct UiState {
    sge::Sprite cursor_foreground;
    sge::Sprite cursor_background;

    std::string fps_text;

    sge::Timer fps_update_timer;

    std::multiset<Element, ElementSort> elements;

    sge::LinearRgba cursor_foreground_color;
    sge::LinearRgba cursor_background_color;

    LLGL::DynamicArray<float> frametime_records;
    uint16_t frametime_record_index = 0;
    float frametime_record_sum = 0.0f;

    float cursor_anim_progress;
    float cursor_scale = 1.0f;

    float hotbar_slot_anim = 1.0f;

    uint8_t previous_selected_slot = 0;

    bool show_extra_ui = false;
    bool show_fps = false;

    AnimationDirection cursor_anim_dir;
} state;

static void draw_inventory(const Inventory& inventory, const glm::vec2& window_size);
static void update_cursor();

void UI::Init() {
    ZoneScopedN("UI::Init");

    state.fps_update_timer = sge::Timer::from_seconds(0.5f, sge::TimerMode::Repeating);
    state.fps_update_timer.set_finished();

    state.cursor_foreground_color = sge::LinearRgba(1.0, 0.08, 0.58);
    state.cursor_background_color = sge::LinearRgba(0.9, 0.9, 0.9);

    state.cursor_background
        .set_texture(Assets::GetTexture(TextureAsset::UiCursorBackground))
        .set_color(state.cursor_background_color)
        .set_anchor(sge::Anchor::TopLeft)
        .set_outline_color(state.cursor_background_color)
        .set_outline_thickness(0.03);

    state.cursor_foreground
        .set_texture(Assets::GetTexture(TextureAsset::UiCursorForeground))
        .set_color(state.cursor_foreground_color)
        .set_anchor(sge::Anchor::TopLeft);

    state.frametime_records = LLGL::DynamicArray<float>(FRAMETIME_RECORD_MAX_COUNT);
}

static SGE_FORCE_INLINE void select_hotbar_slot(Inventory& inventory, uint8_t slot) {
    state.hotbar_slot_anim = 0.0f;
    state.previous_selected_slot = inventory.selected_slot();
    inventory.set_selected_slot(slot);
}

void UI::FixedUpdate() {}

void UI::PreUpdate(Inventory& inventory) {
    ZoneScopedN("UI::PreUpdate");

    for (const Element& element : state.elements) {
        if (element.rect().contains(sge::Input::MouseScreenPosition())) {
            sge::Input::SetMouseOverUi(true);

            if (sge::Input::JustPressed(sge::MouseButton::Left)) {
                switch (element.type()) {
                case UiElement::HotbarCell:
                    if (state.show_extra_ui) {
                        inventory.take_or_put_item(element.data());
                    } else {
                        select_hotbar_slot(inventory, element.data());
                    }
                    break;
                case UiElement::InventoryCell:
                    if (state.show_extra_ui) {
                        inventory.take_or_put_item(element.data());
                    }
                    break;
                }
                break;
            }
        }
    }
}

void UI::Update(Inventory& inventory) {
    ZoneScopedN("UI::Update");

    update_cursor();

    if (sge::Input::JustPressed(sge::Key::Escape)) {
        state.show_extra_ui = !state.show_extra_ui;
        inventory.return_taken_item();
    }

    if (sge::Input::JustPressed(sge::Key::F10)) {
        state.show_fps = !state.show_fps;
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

    const float frametime = sge::Time::DeltaSeconds();
    state.frametime_record_sum -= state.frametime_records[state.frametime_record_index];
    state.frametime_record_sum += frametime;
    state.frametime_records[state.frametime_record_index] = frametime;
    state.frametime_record_index = (state.frametime_record_index + 1) % FRAMETIME_RECORD_MAX_COUNT;

    if (state.show_fps) {
        if (state.fps_update_timer.tick(sge::Time::Delta()).just_finished()) {
            const int fps = (int) (1.0f / (state.frametime_record_sum / FRAMETIME_RECORD_MAX_COUNT));
            state.fps_text = std::to_string(fps);
        }
    }

    if (state.hotbar_slot_anim < 1.0f) {
        state.hotbar_slot_anim += sge::Time::DeltaSeconds() * 8.0f;
        if (state.hotbar_slot_anim > 1.0f) state.hotbar_slot_anim = 1.0f;
    }
}

void UI::PostUpdate() {
    ZoneScopedN("UI::PostUpdate");

    state.elements.clear();
}

static inline void draw_item(const glm::vec2& item_size, const glm::vec2& position, const Item& item, sge::Order item_order = {}) {
    sge::Sprite item_sprite(Assets::GetItemTexture(item.id));
    item_sprite.set_position(position);
    item_sprite.set_anchor(sge::Anchor::Center);
    item_sprite.set_custom_size(item_size);
    item_sprite.set_color(sge::LinearRgba::white());
    GameRenderer::DrawSpriteUI(item_sprite, item_order);
}

static inline void draw_item_with_stack(const sge::Font& font, const glm::vec2& item_size, float stack_size, const glm::vec2& position, const Item& item, sge::Order item_order = {}, sge::Order stack_order = {}) {
    draw_item(item_size, position, item, item_order);

    if (item.stack > 1) {
        const std::string stack_string = std::to_string(item.stack);
        const sge::RichText text = sge::rich_text(stack_string, stack_size, sge::LinearRgba(0.9f));
        const float a = stack_size / 14.0f;
        const glm::vec2 stack_position = glm::vec2(position.x - 15.0f * a, position.y + 2.5f * a);
        GameRenderer::DrawTextUI(text, stack_position, font, stack_order);
    }
}

void UI::Draw(const sge::Camera& camera, const Player& player) {
    ZoneScopedN("UI::Draw");

    const glm::vec2& window_size = camera.viewport();
    const Inventory& inventory = player.inventory();

    draw_inventory(inventory, window_size);

    const sge::Font& font = Assets::GetFont(FontAsset::AndyBold);

    if (state.show_fps) {
        const sge::RichText text = sge::rich_text(state.fps_text, 22.0f, sge::LinearRgba(0.8f));
        GameRenderer::DrawTextUI(text, glm::vec2(10.0f, window_size.y - 10.0f - 22.0f), font, sge::Order(0, false));
    }

    GameRenderer::DrawSpriteUI(state.cursor_background);
    GameRenderer::DrawSpriteUI(state.cursor_foreground);

    const ItemSlot& taken_item = inventory.taken_item();
    const ItemSlot& selected_item = inventory.get_selected_item();
    const glm::vec2 position = state.cursor_background.position() + state.cursor_background.size();

    if (state.show_extra_ui && taken_item.has_item()) {
        const sge::Texture& texture = Assets::GetItemTexture(taken_item.item->id);
        const glm::vec2 size = glm::vec2(texture.size()) * state.cursor_scale;

        draw_item_with_stack(font, size, 16.0f * state.cursor_scale, position, taken_item.item.value());
    } else if (player.can_use_item() && selected_item.has_item() && !sge::Input::IsMouseOverUi()) {
        const sge::Texture& texture = Assets::GetItemTexture(selected_item.item->id);
        const glm::vec2 size = glm::vec2(texture.size()) * state.cursor_scale;

        draw_item(size, position, *selected_item.item);
    }
}

static void update_cursor() {
    if (state.cursor_anim_progress >= 1.0f) {
        state.cursor_anim_dir = AnimationDirection::Backward;
    } else if (state.cursor_anim_progress <= 0.0f) {
        state.cursor_anim_dir = AnimationDirection::Forward;
    }

    switch (state.cursor_anim_dir) {
    case AnimationDirection::Backward:
        state.cursor_anim_progress -= 2.0f * sge::Time::DeltaSeconds();
        break;
    case AnimationDirection::Forward:
        state.cursor_anim_progress += 2.0f * sge::Time::DeltaSeconds();
        break;
    }

    state.cursor_anim_progress = glm::clamp(state.cursor_anim_progress, 0.0f, 1.0f);

    const float scale = MIN_CURSOR_SCALE + state.cursor_anim_progress * (MAX_CURSOR_SCALE - MIN_CURSOR_SCALE);
    state.cursor_scale = scale;

    state.cursor_background.set_position(sge::Input::MouseScreenPosition());
    state.cursor_foreground.set_position(sge::Input::MouseScreenPosition() + glm::vec2(3.0f));

    state.cursor_background.set_scale(glm::vec2(scale));
    state.cursor_foreground.set_scale(glm::vec2(scale));

    state.cursor_foreground.set_color(state.cursor_foreground_color * (0.7f + 0.3f * state.cursor_anim_progress));
}

inline void draw_inventory_cell(UiElement element_type, uint8_t index, const glm::vec2& size, const glm::vec2& position, TextureAsset texture, sge::Order order) {
    sge::Sprite cell_sprite(Assets::GetTexture(texture));
    cell_sprite.set_position(position);
    cell_sprite.set_anchor(sge::Anchor::TopLeft);
    cell_sprite.set_custom_size(size);
    cell_sprite.set_color(sge::LinearRgba(1.0f, 1.0f, 1.0f, 0.8f));
    uint32_t d = GameRenderer::DrawSpriteUI(cell_sprite, order);

    state.elements.emplace(element_type, d, index, sge::Rect::from_top_left(position, size));
}

static void draw_inventory(const Inventory& inventory, const glm::vec2&) {
    ZoneScopedN("UI::render_inventory");

    static constexpr float TITLE_OFFSET = 4.0f;
    static constexpr float DEFAULT_TEXT_SIZE = 14.0f;

    GameRenderer::BeginOrderMode();

    static constexpr uint32_t inventory_index = 0;
    static constexpr uint32_t item_index = 1;
    static constexpr uint32_t text_index = item_index + 1;

    glm::vec2 offset = glm::vec2(INVENTORY_PADDING, INVENTORY_TITLE_SIZE);
    glm::vec2 item_size = glm::vec2(0.0f);
    glm::vec2 cell_size = glm::vec2(0.0f);

    const ItemSlot& taken_item = inventory.taken_item();
    bool item_is_taken = taken_item.has_item();

    const float hotbar_slot_size_selected = glm::mix(HOTBAR_SLOT_SIZE, HOTBAR_SLOT_SIZE_SELECTED, state.hotbar_slot_anim);
    const float hotbat_slot_size_previously_selected = glm::mix(HOTBAR_SLOT_SIZE_SELECTED, HOTBAR_SLOT_SIZE, state.hotbar_slot_anim);

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

        if (state.show_extra_ui) {
            texture = TextureAsset::UiInventoryHotbar;
            cell_size = glm::vec2(INVENTORY_SLOT_SIZE);
            item_size *= 0.95f;
            text_size *= 1.15f;
        } else if (item_selected) {
            texture = TextureAsset::UiInventorySelected;
            cell_size = glm::vec2(hotbar_slot_size_selected);
            text_size = glm::mix(text_size, text_size * 1.3f, state.hotbar_slot_anim);
            padding.y = (HOTBAR_SLOT_SIZE_SELECTED - hotbar_slot_size_selected) * 0.5f;
        } else {
            const float size = state.previous_selected_slot == i ? hotbat_slot_size_previously_selected : HOTBAR_SLOT_SIZE;

            texture = TextureAsset::UiInventoryBackground;
            item_size *= 0.9f;
            cell_size = glm::vec2(size);
            text_size = state.previous_selected_slot == i ? glm::mix(text_size * 1.3f, text_size, state.hotbar_slot_anim) : text_size;
            padding.y = (HOTBAR_SLOT_SIZE_SELECTED - cell_size.y) * 0.5f;
        }

        draw_inventory_cell(UiElement::HotbarCell, i, cell_size, offset + padding, texture, sge::Order(inventory_index));

        if (item_slot.has_item()) {
            const glm::vec2 position = offset + padding + cell_size * 0.5f;
            draw_item_with_stack(font, item_size, text_size, position, item_slot.item.value(), sge::Order(item_index), sge::Order(text_index));
        }

        // Draw cell index
        if (item_slot.has_item() || state.show_extra_ui) {
            float index_size = text_size;
            float index_color = 0.9f;
            if (state.show_extra_ui && item_selected && !item_is_taken) {
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

    if (state.show_extra_ui) {
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

    if (state.show_extra_ui) {
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

                draw_inventory_cell(UiElement::InventoryCell, index, cell_size, offset, TextureAsset::UiInventoryBackground, sge::Order(inventory_index));

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