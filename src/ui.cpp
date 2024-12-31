#include "ui.hpp"

#include <set>

#include <tracy/Tracy.hpp>

#include "renderer/renderer.hpp"
#include "input.hpp"
#include "assets.hpp"
#include "time/time.hpp"
#include "time/timer.hpp"
#include "types/rich_text.hpp"
#include "utils.hpp"

constexpr float INVENTORY_TITLE_SIZE = 24.0f;
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

    Element(UiElement element_type, uint32_t depth, int data, const math::Rect& rect) :
        m_element_type(element_type),
        m_data(data),
        m_depth(depth),
        m_rect(rect) {}

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
    Sprite cursor_foreground;
    Sprite cursor_background;

    std::string fps_text;

    Timer fps_update_timer;

    std::multiset<Element, ElementSort> elements;

    glm::vec3 cursor_foreground_color;
    glm::vec3 cursor_background_color;

    float cursor_anim_progress;

    bool show_extra_ui = false;
    bool show_fps = false;

    AnimationDirection cursor_anim_dir;
} state;

void render_inventory(const Inventory& inventory, const glm::vec2& window_size);
void update_cursor();

void UI::Init() {
    ZoneScopedN("UI::Init");

    state.fps_update_timer = Timer(duration::seconds_float(0.5f), TimerMode::Repeating);
    state.fps_update_timer.set_finished();

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

void UI::FixedUpdate() {
    if (state.show_fps) {
        const float delta = Time::delta_seconds();
        if (delta > 0.0f && state.fps_update_timer.tick(Time::fixed_delta()).just_finished()) {
            const int fps = (int) (1.0f / delta);
            state.fps_text = std::to_string(fps);
        }
    }
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

    if (Input::JustPressed(Key::F10)) {
        state.show_fps = !state.show_fps;
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

    for (const float scroll : Input::ScrollEvents()) {
        const int next_index = static_cast<int>(inventory.selected_slot()) - static_cast<int>(glm::sign(scroll));
        const int new_index = (next_index % CELLS_IN_ROW + CELLS_IN_ROW) % CELLS_IN_ROW;
        inventory.set_selected_slot(static_cast<uint8_t>(new_index));
    }
}

void UI::PostUpdate() {
    ZoneScopedN("UI::PostUpdate");

    state.elements.clear();
}

void UI::Draw(const Camera& camera, const Inventory& inventory) {
    ZoneScopedN("UI::Draw");

    const glm::vec2& window_size = camera.viewport();

    render_inventory(inventory, window_size);

    if (state.show_fps) {
        const RichText text = rich_text(state.fps_text, 22.0f, glm::vec3(0.8f));
        Renderer::DrawTextUI(text, glm::vec2(10.0f, window_size.y - 10.0f - 22.0f), FontAsset::AndyBold);
    }

    const int depth = Renderer::GetMainDepthIndex();

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

inline void render_inventory_cell(UiElement element_type, uint8_t index, const glm::vec2& size, const glm::vec2& position, TextureAsset texture, Depth depth) {
    const glm::vec2 pos = INVENTORY_PADDING + position;
    
    const uint32_t d = depth.value < 0 ? Renderer::GetMainDepthIndex() : depth.value;
    state.elements.emplace(element_type, d, index, math::Rect::from_top_left(pos, size));
    
    Sprite cell_sprite;
    cell_sprite.set_position(pos);
    cell_sprite.set_anchor(Anchor::TopLeft);
    cell_sprite.set_custom_size(tl::optional<glm::vec2>(size));
    cell_sprite.set_color(glm::vec4(1.0f, 1.0f, 1.0f, 0.8f));
    cell_sprite.set_texture(Assets::GetTexture(texture));
    Renderer::DrawSpriteUI(cell_sprite, depth);
}

inline void render_cell_item(const glm::vec2& item_size, const glm::vec2& cell_size, const glm::vec2& position, const Texture& texture, Depth depth) {
    Sprite cell_sprite;
    cell_sprite.set_position(INVENTORY_PADDING + position + (cell_size - item_size) * 0.5f);
    cell_sprite.set_anchor(Anchor::TopLeft);
    cell_sprite.set_custom_size(tl::optional<glm::vec2>(item_size));
    cell_sprite.set_texture(texture);
    Renderer::DrawSpriteUI(cell_sprite, depth);
}

void render_inventory(const Inventory& inventory, const glm::vec2&) {
    ZoneScopedN("UI::render_inventory");

    auto offset = glm::vec2(0.0f, INVENTORY_TITLE_SIZE);
    auto item_offset = glm::vec2(0.0f);

    const uint32_t inventory_index = Renderer::GetMainDepthIndex();
    const uint32_t item_index = inventory_index + 1;
    const uint32_t text_index = item_index + 1;

    if (state.show_extra_ui) {
        offset.x = 0.0f;
        offset.y += INVENTORY_SLOT_SIZE + INVENTORY_CELL_MARGIN;

        const glm::vec2 cell_size = glm::vec2(INVENTORY_SLOT_SIZE);

        for (uint8_t y = 1; y < INVENTORY_ROWS; ++y) {
            for (uint8_t x = 0; x < CELLS_IN_ROW; ++x) {
                const uint8_t index = y * CELLS_IN_ROW + x;

                render_inventory_cell(UiElement::InventoryCell, index, cell_size, offset, TextureAsset::UiInventoryBackground, Depth(inventory_index, false));
                offset.x += cell_size.x + INVENTORY_CELL_MARGIN;
            }

            offset.y += cell_size.y + INVENTORY_CELL_MARGIN;
            offset.x = 0.0f;
        }

        offset = glm::vec2(0.0f, INVENTORY_TITLE_SIZE);
        item_offset = glm::vec2(0.0f);
    }

    glm::vec2 item_size = glm::vec2(0.0f);
    glm::vec2 cell_size = glm::vec2(0.0f);

    for (uint8_t i = 0; i < CELLS_IN_ROW; ++i) {
        TextureAsset texture;
        glm::vec2 padding = glm::vec2(0.0f);
        float text_size = 14.0f;

        tl::optional<const Item&> item = inventory.get_item(i);
        const bool item_selected = inventory.selected_slot() == i;

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

        render_inventory_cell(UiElement::HotbarCell, i, cell_size, offset + padding, texture, Depth(inventory_index, false));

        if (item.is_some()) {
            render_cell_item(item_size, cell_size, offset + padding, Assets::GetItemTexture(item->id), Depth(item_index, false));
        }

        // Draw cell index
        if (item.is_some() || state.show_extra_ui) {
            float index_size = text_size;
            float index_color = 0.8f;
            if (state.show_extra_ui && item_selected) {
                index_size = 16.0f;
                index_color = 1.0f;
            }

            const char index = '0' + (i + 1) % 10;

            Renderer::DrawCharUI(index, offset + padding + INVENTORY_PADDING + glm::vec2(5.0f, 5.0f), index_size, glm::vec3(index_color), FontAsset::AndyBold, text_index);
        }

        // Draw item stack
        if (item.is_some() && item->stack > 1) {
            const RichText text = rich_text(std::to_string(item->stack), text_size, glm::vec3(0.8f));
            Renderer::DrawTextUI(text, offset + padding + INVENTORY_PADDING + glm::vec2(5.0f, cell_size.y - text_size - 2.5f), FontAsset::AndyBold, text_index);
        }

        offset.x += cell_size.x + INVENTORY_CELL_MARGIN;
        item_offset.x += item_size.x + INVENTORY_CELL_MARGIN;
    }

    if (state.show_extra_ui) {
        const RichText text = rich_text("Inventory", INVENTORY_TITLE_SIZE, glm::vec3(0.8f));
        Renderer::DrawTextUI(text, glm::vec2(INVENTORY_PADDING + INVENTORY_SLOT_SIZE * 0.5f, INVENTORY_TITLE_SIZE * 0.5f), FontAsset::AndyBold, inventory_index);
    } else {
        auto item = inventory.get_item(inventory.selected_slot());
        const std::string& title = item.is_some() ? item->name : "Items";
        const RichText text = rich_text(title, INVENTORY_TITLE_SIZE, glm::vec3(0.8f));

        const glm::vec2 bounds = calculate_text_bounds(title, INVENTORY_TITLE_SIZE, FontAsset::AndyBold);
        Renderer::DrawTextUI(text, glm::vec2((offset.x - bounds.x) * 0.5f, INVENTORY_TITLE_SIZE * 0.5f), FontAsset::AndyBold, inventory_index);
    }

    // Draw pangramas in different languages to test text rendering
    // const RichText text = rich_text(
    //     RichTextSection("The quick brown fox jumps over the lazy dog æ\n", 48.0f, glm::vec3(0.9f, 0.4f, 0.4f)),
    //     RichTextSection("Съешь ещё этих мягких французских булок, да выпей же чаю\n", 48.0f, glm::vec3(0.4f, 0.9f, 0.4f))
    // );
    // const glm::vec2 bounds = calculate_text_bounds(text, FontAsset::AndyBold);
    // Renderer::DrawTextUI(text, glm::vec2(window_size.x * 0.5f - bounds.x * 0.5f, window_size.y * 0.5f - bounds.y * 0.5f), FontAsset::AndyBold, inventory_index);
}