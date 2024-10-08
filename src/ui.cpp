#include "ui.hpp"

#include "renderer/renderer.hpp"
#include "input.hpp"
#include "assets.hpp"
#include "time/time.hpp"
#include "utils.hpp"

static struct UiState {
    bool show_extra_ui = false;

    float cursor_anim_progress;
    AnimationDirection cursor_anim_dir;

    glm::vec3 cursor_foreground_color;
    glm::vec3 cursor_background_color;

    Sprite cursor_foreground;
    Sprite cursor_background;
    
    std::vector<Element> elements;
} state;

void render_inventory(const Inventory& inventory);
void update_cursor();

void UI::Init() {
    state.cursor_foreground_color = glm::vec3(1.0, 0.08, 0.58);
    state.cursor_background_color = glm::vec3(0.9, 0.9, 0.9);

    state.cursor_background
        .set_texture(Assets::GetTexture(TextureAsset::UiCursorBackground))
        .set_color(state.cursor_background_color)
        .set_anchor(Anchor::TopLeft)
        .set_outline_color(state.cursor_background_color)
        .set_outline_thickness(0.03);

    state.cursor_foreground
        .set_texture(Assets::GetTexture(TextureAsset::UiCursorForeground))
        .set_color(state.cursor_foreground_color)
        .set_anchor(Anchor::TopLeft);

    state.elements.reserve(100);
}

void UI::PreUpdate(Inventory& inventory) {
    for (const Element& element : state.elements) {
        if (element.rect().contains(Input::MouseScreenPosition())) {
            Input::SetMouseOverUi(true);
            
            if (Input::JustPressed(MouseButton::Left)) {
                switch (element.type()) {
                case UiElement::HotbarCell:
                    inventory.set_selected_slot(element.data());
                    break;
                case UiElement::InventoryCell:
                    break;
                }
            }
        }
    }
}

void UI::Update(Inventory& inventory) {
    update_cursor();

    if (Input::JustPressed(Key::Escape)) {
        state.show_extra_ui = !state.show_extra_ui;
    }

    if (Input::JustPressed(Key::Digit1)) inventory.set_selected_slot(0);
    if (Input::JustPressed(Key::Digit2)) inventory.set_selected_slot(1);
    if (Input::JustPressed(Key::Digit3)) inventory.set_selected_slot(2);
    if (Input::JustPressed(Key::Digit4)) inventory.set_selected_slot(3);
    if (Input::JustPressed(Key::Digit5)) inventory.set_selected_slot(4);
    if (Input::JustPressed(Key::Digit6)) inventory.set_selected_slot(5);
    if (Input::JustPressed(Key::Digit7)) inventory.set_selected_slot(6);
    if (Input::JustPressed(Key::Digit8)) inventory.set_selected_slot(7);
    if (Input::JustPressed(Key::Digit9)) inventory.set_selected_slot(8);
    if (Input::JustPressed(Key::Digit0)) inventory.set_selected_slot(9);

    for (float scroll : Input::ScrollEvents()) {
        int next_index = static_cast<int>(inventory.selected_slot()) - static_cast<int>(glm::sign(scroll));
        int new_index = (next_index % CELLS_IN_ROW + CELLS_IN_ROW) % CELLS_IN_ROW;
        inventory.set_selected_slot(static_cast<uint8_t>(new_index));
    }
}

void UI::PostUpdate() {
    state.elements.clear();
}

void UI::Render(const Camera&, const Inventory& inventory) {
    render_inventory(inventory);

    Renderer::DrawSpriteUI(state.cursor_background);

    // Sprite shadow = state.cursor_foreground;
    // shadow.set_color(glm::vec4(0.5, 0.5, 0.5, 0.8));
    // shadow.set_position(shadow.position() + glm::vec2(2.0f));
    // Renderer::DrawSpriteUI(shadow);

    Renderer::DrawSpriteUI(state.cursor_foreground);
}

void update_cursor() {
    if (state.cursor_anim_progress >= 1.0f) {
        state.cursor_anim_dir = AnimationDirection::Backward;
    } else if (state.cursor_anim_progress <= 0.0f) {
        state.cursor_anim_dir = AnimationDirection::Forward;
    }

    switch (state.cursor_anim_dir) {
    case AnimationDirection::Backward:
        state.cursor_anim_progress -= 1.5f * Time::delta_seconds();
        break;
    case AnimationDirection::Forward:
        state.cursor_anim_progress += 1.5f * Time::delta_seconds();
        break;
    }

    state.cursor_anim_progress = glm::clamp(state.cursor_anim_progress, 0.0f, 1.0f);

    const float scale = MIN_CURSOR_SCALE + state.cursor_anim_progress * (MAX_CURSOR_SCALE - MIN_CURSOR_SCALE);

    state.cursor_background.set_position(Input::MouseScreenPosition());
    state.cursor_foreground.set_position(Input::MouseScreenPosition() + glm::vec2(2.0f));

    state.cursor_background.set_scale(glm::vec2(scale));
    state.cursor_foreground.set_scale(glm::vec2(scale));

    state.cursor_foreground.set_color(state.cursor_foreground_color * (0.7f + 0.3f * state.cursor_anim_progress));
}

inline void render_inventory_cell(UiElement element_type, uint8_t index, const glm::vec2& size, const glm::vec2& position, TextureAsset texture, int depth) {
    const glm::vec2 pos = INVENTORY_PADDING + position;
    
    state.elements.emplace_back(element_type, index, math::Rect::from_top_left(pos, size));
    
    Sprite cell_sprite;
    cell_sprite.set_position(pos);
    cell_sprite.set_anchor(Anchor::TopLeft);
    cell_sprite.set_custom_size(tl::optional<glm::vec2>(size));
    cell_sprite.set_color(glm::vec4(1.0f, 1.0f, 1.0f, 0.8f));
    cell_sprite.set_texture(Assets::GetTexture(texture));
    Renderer::DrawSpriteUI(cell_sprite, depth);
}

inline void render_cell_item(const glm::vec2& item_size, const glm::vec2& cell_size, const glm::vec2& position, const Texture& texture, int depth) {
    Sprite cell_sprite;
    cell_sprite.set_position(INVENTORY_PADDING + position + (cell_size - item_size) * 0.5f);
    cell_sprite.set_anchor(Anchor::TopLeft);
    cell_sprite.set_custom_size(tl::optional<glm::vec2>(item_size));
    cell_sprite.set_texture(texture);
    Renderer::DrawSpriteUI(cell_sprite, depth);
}

void render_inventory(const Inventory& inventory) {
    auto offset = glm::vec2(0.0f, 18.0f);
    auto item_offset = glm::vec2(0);

    const int cell_depth = Renderer::GetUiDepthIndex();
    const int item_depth = Renderer::GetUiDepthIndex() + 1;
    const int text_depth = Renderer::GetUiDepthIndex() + 2;

    for (size_t x = 0; x < CELLS_IN_ROW; ++x) {
        TextureAsset texture;
        auto item_size = glm::vec2(0.0f);
        auto cell_size = glm::vec2(0.0f);
        auto padding = glm::vec2(0.0f);
        float text_size = 14.0f;

        auto item = inventory.get_item(x);
        const bool item_selected = inventory.selected_slot() == x;

        if (item.is_some()) {
            item_size = Assets::GetItemTexture(item->id).size;
            item_size = glm::min(item_size, glm::vec2(32.0f)); // The maximum size of an item image is 32px
        }

        if (state.show_extra_ui) {
            texture = TextureAsset::UiInventoryHotbar;
            cell_size = glm::vec2(INVENTORY_SLOT_SIZE);
            item_size *= 0.95f;
        } else if (item_selected) {
            texture = TextureAsset::UiInventorySelected;
            cell_size = glm::vec2(HOTBAR_SLOT_SIZE_SELECTED);
            text_size = 16.0f;
        } else {
            texture = TextureAsset::UiInventoryBackground;
            item_size *= 0.9f;
            cell_size = glm::vec2(HOTBAR_SLOT_SIZE);
            padding.y = (HOTBAR_SLOT_SIZE_SELECTED - cell_size.y) * 0.5f;
        }

        render_inventory_cell(UiElement::HotbarCell, x, cell_size, offset + padding, texture, cell_depth);

        if (item.is_some()) {
            render_cell_item(item_size, cell_size, offset + padding, Assets::GetItemTexture(item->id), item_depth);
        }

        // Draw cell index
        if (item.is_some() || state.show_extra_ui) {
            float index_size = text_size;
            float index_color = 0.8f;
            if (state.show_extra_ui && item_selected) {
                index_size = 16.0f;
                index_color = 1.0f;
            }

            const char index = '0' + (x + 1) % 10;

            Renderer::DrawCharUi(index, index_size, offset + padding + glm::vec2(16.0f, text_size * 1.7f), glm::vec3(index_color), FontAsset::AndyBold, text_depth);
        }

        // Draw item stack
        if (item.is_some() && item->stack > 1) {
            Renderer::DrawTextUi(std::to_string(item->stack), text_size, offset + padding + glm::vec2(16.0f, cell_size.y * 0.5f + text_size * 1.6f), glm::vec3(0.8f), FontAsset::AndyBold, text_depth);
        }

        offset.x += cell_size.x + INVENTORY_CELL_MARGIN;
        item_offset.x += item_size.x + INVENTORY_CELL_MARGIN;
    }

    if (state.show_extra_ui) {
        Renderer::DrawTextUi("Inventory", 22.0f, glm::vec2(INVENTORY_PADDING + INVENTORY_SLOT_SIZE * 0.5f, 22.0f), glm::vec3(0.8f), FontAsset::AndyBold, text_depth);
    } else {
        auto item = inventory.get_item(inventory.selected_slot());
        const std::string& title = item.is_some() ? item->name : "Items";

        glm::vec2 bounds = calculate_text_bounds(FontAsset::AndyBold, title, 22.0f);
        Renderer::DrawTextUi(title, 22.0f, glm::vec2((offset.x - bounds.x) * 0.5f, 22.0f), glm::vec3(0.8f), FontAsset::AndyBold, text_depth);
    }

    if (state.show_extra_ui) {
        offset.x = 0;
        offset.y += INVENTORY_SLOT_SIZE + INVENTORY_CELL_MARGIN;

        const glm::vec2 cell_size = glm::vec2(state.show_extra_ui ? INVENTORY_SLOT_SIZE : HOTBAR_SLOT_SIZE);

        for (uint8_t y = 1; y < INVENTORY_ROWS; ++y) {
            for (uint8_t x = 0; x < CELLS_IN_ROW; ++x) {
                const uint8_t index = y * CELLS_IN_ROW + x;

                render_inventory_cell(UiElement::InventoryCell, index, cell_size, offset, TextureAsset::UiInventoryBackground, cell_depth);
                offset.x += cell_size.x + INVENTORY_CELL_MARGIN;
            }

            offset.y += cell_size.y + INVENTORY_CELL_MARGIN;
            offset.x = 0;
        }
    }
}