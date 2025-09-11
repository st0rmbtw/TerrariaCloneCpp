#include "ui.hpp"
#include "SGE/assert.hpp"
#include "SGE/input.hpp"
#include "SGE/math/rect.hpp"

inline constexpr size_t MAX_LAYOUT_STACK_SIZE = 100;

struct Layout {
    glm::vec2 size = glm::vec2(0.0f);
    glm::vec2 pos = glm::vec2(0.0f);
    UiRect padding;
    float gap = 0.0f;
    LayoutOrientation orientation = LayoutOrientation::Horizontal;
    Alignment alignment = Alignment::Start;
};

static struct {
    Layout layout_stack[MAX_LAYOUT_STACK_SIZE];
    size_t layout_stack_size = 0;

    glm::vec2 layout_size = glm::vec2(0.0f);

    uint32_t active_id = 0;
    uint32_t next_id = 0;

    bool any_hovered = false;
} state;

static Layout& TopLayout() {
    SGE_ASSERT(state.layout_stack_size > 0);

    return state.layout_stack[state.layout_stack_size-1];
}

static glm::vec2 GetAvailablePos(const Layout& layout) {
    switch (layout.orientation) {
    case LayoutOrientation::Horizontal:
        return layout.padding.left() + layout.pos + layout.size * glm::vec2(1.0f, 0.0f);
    case LayoutOrientation::Vertical:
        return layout.padding.top() + layout.pos + layout.size * glm::vec2(0.0f, 1.0f);
    }
}

static Layout PopLayout() {
    return state.layout_stack[--state.layout_stack_size];
}

static glm::vec2 GetLayoutSize(const Layout& layout) {
    return layout.size + glm::vec2(layout.padding.right(), layout.padding.bottom());
}

static void LayoutAdvance(Layout& layout, const glm::vec2& size) {
    switch (layout.orientation) {
    case LayoutOrientation::Horizontal:
        layout.size.x += size.x + layout.gap;
        layout.size.y = std::max(layout.size.y, size.y);
        break;
    case LayoutOrientation::Vertical:
        layout.size.x = std::max(layout.size.x, size.x);
        layout.size.y += size.y + layout.gap;
        break;
    }
}

void UI::BeginLayout(const LayoutDesc& layout) {
    SGE_ASSERT(state.layout_stack_size < MAX_LAYOUT_STACK_SIZE);

    glm::vec2 pos = glm::vec2(0.0f);

    if (state.layout_stack_size > 0) {
        pos = GetAvailablePos(TopLayout());
    }

    state.layout_stack[state.layout_stack_size] = Layout {
        .pos = pos,
        .padding = layout.padding(),
        .gap = layout.gap(),
        .orientation = layout.orientation()
    };
    state.layout_stack_size += 1;
}

void UI::EndLayout() {
    SGE_ASSERT(state.layout_stack_size > 0);

    const Layout& layout = PopLayout();

    if (state.layout_stack_size == 0) {
        state.active_id = 0;
        state.next_id = 0;
        state.any_hovered = false;
    } else {
        Layout& top_layout = TopLayout();
        LayoutAdvance(top_layout, layout.size + glm::vec2(top_layout.padding.right(), top_layout.padding.bottom()));
    }
}

[[nodiscard]]
bool UI::IsMouseOverUi() {
    return state.any_hovered;
}

ElementState UI::Element(const glm::vec2& size) {
    ElementState element_state;

    uint32_t id = state.next_id++;

    Layout& top_layout = TopLayout();

    const glm::vec2 available_pos = GetAvailablePos(top_layout);

    sge::Rect element_rect = sge::Rect::from_top_left(available_pos, size);

    const bool hovered = element_rect.contains(sge::Input::MouseScreenPosition());

    state.any_hovered = state.any_hovered || hovered;

    if (state.active_id == 0) {
        if (hovered && sge::Input::JustPressed(sge::MouseButton::Left)) {
            state.active_id = id;
        }
    } else if (state.active_id == id) {
        if (sge::Input::JustReleased(sge::MouseButton::Left)) {
            state.active_id = 0;
            if (hovered)
                element_state.interaction |= Interaction::Pressed;
        }
    }

    LayoutAdvance(top_layout, size);

    element_state.position = available_pos;
    element_state.interaction |= hovered ? Interaction::Hovered : 0;

    return element_state;
}