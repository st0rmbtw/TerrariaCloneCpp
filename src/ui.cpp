#include "ui.hpp"
#include "renderer/renderer.hpp"
#include "input.hpp"
#include "assets.hpp"

static struct UiState {
    bool show_extra_ui = false;
} state;

void render_inventory(const Inventory& inventory);

void UI::Update(Inventory& inventory) {
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

void UI::Render(const Camera& camera, const Inventory& inventory) {
    render_inventory(inventory);

    Sprite sprite(Input::MouseScreenPosition());
    sprite.set_custom_size(glm::vec2(25.0f));
    sprite.set_color(glm::vec3(1.0f, 0.0f, 0.0f));
    sprite.set_anchor(Anchor::TopLeft);
    sprite.set_texture(Assets::GetTexture(AssetKey::TextureUiCursorForeground));
    sprite.set_order(100);

    Renderer::DrawSprite(sprite, RenderLayer::UI);
}

inline void render_inventory_cell(UiElement element_type, uint8_t index, const glm::vec2& size, const glm::vec2& position, const Texture& texture) {
    const glm::vec2 pos = INVENTORY_PADDING + position;
    
    // m_elements.push_back(Element(element_type, index, Rect::from_top_left(pos, size)));
    
    Sprite cell_sprite(pos);
    cell_sprite.set_anchor(Anchor::TopLeft);
    cell_sprite.set_custom_size(tl::optional<glm::vec2>(size));
    cell_sprite.set_color(glm::vec4(1.0f, 1.0f, 1.0f, 0.8f));
    cell_sprite.set_texture(texture);
    Renderer::DrawSprite(cell_sprite, RenderLayer::UI);
}

inline void render_cell_item(const glm::vec2& item_size, const glm::vec2& cell_size, const glm::vec2& position, const Texture& texture) {
    Sprite cell_sprite(INVENTORY_PADDING + position + (cell_size - item_size) / 2.0f);
    cell_sprite.set_anchor(Anchor::TopLeft);
    cell_sprite.set_custom_size(tl::optional<glm::vec2>(item_size));
    cell_sprite.set_texture(texture);
    Renderer::DrawSprite(cell_sprite, RenderLayer::UI);
}

void render_inventory(const Inventory& inventory) {
    auto offset = glm::vec2(0.0f, 18.0f);
    auto item_offset = glm::vec2(0);

    for (size_t x = 0; x < CELLS_IN_ROW; ++x) {
        Texture texture;
        auto item_size = glm::vec2(0.0f);
        auto cell_size = glm::vec2(0.0f);
        auto padding = glm::vec2(0.0f);
        float text_size = 14.0f;

        auto item = inventory.get_item(x);
        const bool item_selected = inventory.selected_slot() == x;

        if (item.is_some()) {
            item_size = Assets::GetItemTexture(item->id).size();
            item_size = glm::min(item_size, glm::vec2(32.0f)); // The maximum size of an item image is 32px
        }

        if (state.show_extra_ui) {
            texture = Assets::GetTexture(AssetKey::TextureUiInventoryHotbar);
            cell_size = glm::vec2(INVENTORY_SLOT_SIZE);
            item_size *= 0.95f;
        } else if (item_selected) {
            texture = Assets::GetTexture(AssetKey::TextureUiInventorySelected);
            cell_size = glm::vec2(HOTBAR_SLOT_SIZE_SELECTED);
            text_size = 16.0f;
        } else {
            texture = Assets::GetTexture(AssetKey::TextureUiInventoryBackground);
            item_size *= 0.9f;
            cell_size = glm::vec2(HOTBAR_SLOT_SIZE);
            padding.y = (HOTBAR_SLOT_SIZE_SELECTED - cell_size.y) / 2.0f;
        }

        render_inventory_cell(UiElement::HotbarCell, x, cell_size, offset + padding, texture);

        if (item.is_some()) {
            render_cell_item(item_size, cell_size, offset + padding, Assets::GetItemTexture(item->id));
        }

        // Draw cell index
        if (item.is_some() || state.show_extra_ui) {
            float index_size = text_size;
            float index_color = 0.8f;
            if (state.show_extra_ui && item_selected) {
                index_size = 16.0f;
                index_color = 1.0f;
            }

            // Renderer::draw_text_ui(FontKey::AndyBold, std::to_string((x + 1) % 10), offset + padding + glm::vec2(16.0f, text_size * 1.7f), index_size, glm::vec3(index_color));
        }

        // Draw item stack
        if (item.is_some() && item->stack > 1) {
            // Renderer::draw_text_ui(FontKey::AndyBold, std::to_string(item->stack), offset + padding + glm::vec2(16.0f, cell_size.y / 2.0f + text_size * 1.6f), text_size, glm::vec3(0.8f));
        }

        offset.x += cell_size.x + INVENTORY_CELL_MARGIN;
        item_offset.x += item_size.x + INVENTORY_CELL_MARGIN;
    }

    // if (state.show_extra_ui) {
    //     Renderer::draw_text_ui(FontKey::AndyBold, "Inventory", glm::vec2(INVENTORY_PADDING + INVENTORY_SLOT_SIZE / 2.0f, 22.0f), 22.0f, glm::vec3(0.8f));
    // } else {
    //     auto item = inventory.get_item(inventory.selected_slot());
    //     const std::string title = item.is_some() ? item->name : "Items";

    //     glm::vec2 bounds = calculate_text_bounds(FontKey::AndyBold, title, 22.0f);
    //     Renderer::draw_text_ui(FontKey::AndyBold, title, glm::vec2(offset.x / 2.0f - bounds.x / 2.0f, 22.0f), 22.0f, glm::vec3(0.8f));
    // }

    if (state.show_extra_ui) {
        offset.x = 0;
        offset.y += INVENTORY_SLOT_SIZE + INVENTORY_CELL_MARGIN;

        const glm::vec2 cell_size = glm::vec2(state.show_extra_ui ? INVENTORY_SLOT_SIZE : HOTBAR_SLOT_SIZE);

        for (size_t y = 1; y < INVENTORY_ROWS; ++y) {
            for (size_t x = 0; x < CELLS_IN_ROW; ++x) {
                const size_t index = y * CELLS_IN_ROW + x;

                render_inventory_cell(UiElement::InventoryCell, index, cell_size, offset, Assets::GetTexture(AssetKey::TextureUiInventoryBackground));
                offset.x += cell_size.x + INVENTORY_CELL_MARGIN;
            }

            offset.y += cell_size.y + INVENTORY_CELL_MARGIN;
            offset.x = 0;
        }
    }
}