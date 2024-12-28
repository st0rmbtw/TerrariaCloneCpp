#include "ui.hpp"

#include <set>
#include <utility>

#include <tracy/Tracy.hpp>

#include "renderer/renderer.hpp"
#include "input.hpp"
#include "assets.hpp"
#include "time/time.hpp"
#include "utils.hpp"

constexpr float INVENTORY_TITLE_SIZE = 22.0f;
constexpr float MIN_CURSOR_SCALE = 1.0;
constexpr float MAX_CURSOR_SCALE = MIN_CURSOR_SCALE + 0.20;
constexpr float INVENTORY_PADDING = 10;
constexpr float HOTBAR_SLOT_SIZE = 40;
constexpr float INVENTORY_SLOT_SIZE = HOTBAR_SLOT_SIZE * 1.1;
constexpr float HOTBAR_SLOT_SIZE_SELECTED = HOTBAR_SLOT_SIZE * 1.3;
constexpr float INVENTORY_CELL_MARGIN = 4;

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

    Element(UiElement element_type, uint32_t depth, int data, math::Rect rect) :
        m_element_type(element_type),
        m_data(data),
        m_depth(depth),
        m_rect(std::move(rect)) {}

    inline void press() noexcept { m_state = m_state | Pressed; }
    inline void hover() noexcept { m_state = m_state | Hovered; }

    [[nodiscard]] inline bool none() const noexcept { return m_state == None; }
    [[nodiscard]] inline bool hovered() const noexcept { return m_state & Hovered; }
    [[nodiscard]] inline bool pressed() const noexcept { return m_state & Pressed; }

    [[nodiscard]] inline const math::Rect& rect() const noexcept { return m_rect; }
    [[nodiscard]] inline UiElement type() const noexcept { return m_element_type; }
    [[nodiscard]] inline int data() const noexcept { return m_data; }
    [[nodiscard]] inline int depth() const noexcept { return m_depth; }
private:
    uint8_t m_state = 0;
    UiElement m_element_type;
    int m_data = 0;
    uint32_t m_depth = 0;
    math::Rect m_rect;
};

struct ElementSort {
    constexpr bool operator()(const Element& a, const Element& b) const {
        return a.depth() > b.depth();
    }
};

static struct UiState {
    bool show_extra_ui = false;

    float cursor_anim_progress;
    AnimationDirection cursor_anim_dir;

    glm::vec3 cursor_foreground_color;
    glm::vec3 cursor_background_color;

    Sprite cursor_foreground;
    Sprite cursor_background;
    
    std::set<Element, ElementSort> elements;
} state;

void render_inventory(const Inventory& inventory);
void update_cursor();

void UI::Init() {
    ZoneScopedN("UI::Init");

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
}

void UI::PreUpdate(Inventory& inventory) {
    ZoneScopedN("UI::PreUpdate");

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
    ZoneScopedN("UI::Update");

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
    ZoneScopedN("UI::PostUpdate");

    state.elements.clear();
}

void UI::Draw(const Camera&, const Inventory& inventory) {
    ZoneScopedN("UI::Draw");

    render_inventory(inventory);

    const int depth = Renderer::GetUiDepthIndex();

    Renderer::DrawSpriteUI(state.cursor_background, depth);

    // Sprite shadow = state.cursor_foreground;
    // shadow.set_color(glm::vec4(0.5, 0.5, 0.5, 0.8));
    // shadow.set_position(shadow.position() + glm::vec2(2.0f));
    // Renderer::DrawSpriteUI(shadow);

    Renderer::DrawSpriteUI(state.cursor_foreground, depth + 1);
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

inline void render_inventory_cell(UiElement element_type, uint8_t index, const glm::vec2& size, const glm::vec2& position, TextureAsset texture) {
    const glm::vec2 pos = INVENTORY_PADDING + position;

    const uint32_t depth = Renderer::GetUiDepthIndex();
    
    state.elements.emplace(element_type, depth, index, math::Rect::from_top_left(pos, size));
    
    Sprite cell_sprite;
    cell_sprite.set_position(pos);
    cell_sprite.set_anchor(Anchor::TopLeft);
    cell_sprite.set_custom_size(tl::optional<glm::vec2>(size));
    cell_sprite.set_color(glm::vec4(1.0f, 1.0f, 1.0f, 0.8f));
    cell_sprite.set_texture(Assets::GetTexture(texture));
    Renderer::DrawSpriteUI(cell_sprite);
}

inline void render_cell_item(const glm::vec2& item_size, const glm::vec2& cell_size, const glm::vec2& position, const Texture& texture) {
    Sprite cell_sprite;
    cell_sprite.set_position(INVENTORY_PADDING + position + (cell_size - item_size) * 0.5f);
    cell_sprite.set_anchor(Anchor::TopLeft);
    cell_sprite.set_custom_size(tl::optional<glm::vec2>(item_size));
    cell_sprite.set_texture(texture);
    Renderer::DrawSpriteUI(cell_sprite);
}

void render_inventory(const Inventory& inventory) {
    ZoneScopedN("UI::render_inventory");

    auto offset = glm::vec2(0.0f, INVENTORY_TITLE_SIZE);
    auto item_offset = glm::vec2(0.0f);

    if (state.show_extra_ui) {
        offset.x = 0.0f;
        offset.y += INVENTORY_SLOT_SIZE + INVENTORY_CELL_MARGIN;

        const glm::vec2 cell_size = glm::vec2(INVENTORY_SLOT_SIZE);

        for (uint8_t y = 1; y < INVENTORY_ROWS; ++y) {
            for (uint8_t x = 0; x < CELLS_IN_ROW; ++x) {
                const uint8_t index = y * CELLS_IN_ROW + x;

                render_inventory_cell(UiElement::InventoryCell, index, cell_size, offset, TextureAsset::UiInventoryBackground);
                offset.x += cell_size.x + INVENTORY_CELL_MARGIN;
            }

            offset.y += cell_size.y + INVENTORY_CELL_MARGIN;
            offset.x = 0.0f;
        }
    }

    offset = glm::vec2(0.0f, INVENTORY_TITLE_SIZE);
    item_offset = glm::vec2(0.0f);

    glm::vec2 item_size = glm::vec2(0.0f);
    glm::vec2 cell_size = glm::vec2(0.0f);

    for (size_t x = 0; x < CELLS_IN_ROW; ++x) {
        TextureAsset texture;
        glm::vec2 padding = glm::vec2(0.0f);
        float text_size = 14.0f;

        tl::optional<const Item&> item = inventory.get_item(x);
        const bool item_selected = inventory.selected_slot() == x;

        if (item.is_some()) {
            item_size = Assets::GetItemTexture(item->id).size();
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

            const char index = '0' + (x + 1) % 10;

            Renderer::DrawCharUI(index, index_size, offset + padding + glm::vec2(16.0f, text_size * 1.7f), glm::vec3(index_color), FontAsset::AndyBold);
        }

        // Draw item stack
        if (item.is_some() && item->stack > 1) {
            Renderer::DrawTextUI(std::to_string(item->stack), text_size, offset + padding + glm::vec2(16.0f, cell_size.y * 0.5f + text_size * 1.6f), glm::vec3(0.8f), FontAsset::AndyBold);
        }

        offset.x += cell_size.x + INVENTORY_CELL_MARGIN;
        item_offset.x += item_size.x + INVENTORY_CELL_MARGIN;
    }

    if (state.show_extra_ui) {
        Renderer::DrawTextUI("Inventory", INVENTORY_TITLE_SIZE, glm::vec2(INVENTORY_PADDING + INVENTORY_SLOT_SIZE * 0.5f, 22.0f), glm::vec3(0.8f), FontAsset::AndyBold);
    } else {
        auto item = inventory.get_item(inventory.selected_slot());
        const std::string& title = item.is_some() ? item->name : "Items";

        glm::vec2 bounds = calculate_text_bounds(FontAsset::AndyBold, title, INVENTORY_TITLE_SIZE);
        Renderer::DrawTextUI(title, INVENTORY_TITLE_SIZE, glm::vec2((offset.x - bounds.x) * 0.5f, 22.0f), glm::vec3(0.8f), FontAsset::AndyBold);
    }
}