#include <cstddef>
#include <vector>
#include <new>

#include <SGE/assert.hpp>
#include <SGE/input.hpp>
#include <SGE/math/rect.hpp>
#include <SGE/utils/containers/swapbackvector.hpp>

#include "ui.hpp"

inline constexpr size_t MAX_NODE_STACK_SIZE = 100;
inline constexpr size_t ARENA_SIZE = 1024 * 1024;

class Arena {
public:
    explicit Arena(size_t capacity) : m_capacity(capacity) {
        m_data = new uint8_t[capacity];
    }

    // Allocate raw memory from the arena
    void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) {
        const size_t aligned_offset = (m_current_offset + alignment - 1) & ~(alignment - 1);

        SGE_ASSERT(aligned_offset + size <= m_capacity);

        void* ptr = m_data + aligned_offset;
        m_current_offset = aligned_offset + size;
        return ptr;
    }

    void clear() {
        m_current_offset = 0;
    }

private:
    uint8_t* m_data = nullptr;
    size_t m_current_offset = 0;
    size_t m_capacity = 0;
};

struct Node {
    std::vector<size_t> children;

    glm::vec2 pos = glm::vec2(0.0f);
    glm::vec2 size = glm::vec2(0.0f);

    UiRect padding;
    UiSize sizing;

    void* custom_data = nullptr;
    size_t custom_data_size = 0;

    float gap = 0.0f;
    
    uint32_t unique_id = 0;
    uint32_t type_id = ReservedTypeID::Container;

    uint32_t z_index = 0;
    
    LayoutOrientation orientation = LayoutOrientation::Horizontal;
    Alignment horizontal_alignment = Alignment::Start;
    Alignment vertical_alignment = Alignment::Start;
};

static struct {
    size_t node_stack[MAX_NODE_STACK_SIZE];
    size_t node_stack_size = 0;

    Arena arena{ ARENA_SIZE };

    std::vector<Node> nodes{};
    std::vector<UiElement> render_elements{};

    std::vector<uint32_t> search_visited{};
    sge::SwapbackVector<Node*> search_stack{};

    uint32_t active_id = 0;
    uint32_t next_id = 0;

    bool any_hovered = false;
} state;

static Node& TopNode() {
    SGE_ASSERT(state.node_stack_size > 0);

    const size_t index = state.node_stack[state.node_stack_size-1];
    return state.nodes[index];
}

static void NodeAddElement(Node& parent, Node&& child) {
    const size_t index = state.nodes.size();
    parent.children.push_back(index);
    
    state.node_stack[state.node_stack_size] = index;
    state.node_stack_size += 1;

    state.nodes.push_back(std::move(child));
}

static size_t PushNode(Node&& node) {
    const size_t index = state.nodes.size();
    state.nodes.push_back(std::move(node));

    state.node_stack[state.node_stack_size] = index;
    state.node_stack_size += 1;

    return index;
}

[[nodiscard]]
bool UI::IsMouseOverUi() {
    return state.any_hovered;
}

static Interaction::Type CheckInteraction(uint32_t id, const glm::vec2& pos, const glm::vec2& size) {
    const sge::Rect element_rect = sge::Rect::from_top_left(pos, size);

    const bool hovered = element_rect.contains(sge::Input::MouseScreenPosition());

    state.any_hovered = state.any_hovered || hovered;

    Interaction::Type interaction = hovered ? Interaction::Hovered : Interaction::None;

    if (state.active_id == 0) {
        if (hovered && sge::Input::JustPressed(sge::MouseButton::Left)) {
            state.active_id = id;
        }
    } else if (state.active_id == id) {
        if (sge::Input::JustReleased(sge::MouseButton::Left)) {
            state.active_id = 0;
            if (hovered) {
                interaction |= Interaction::Pressed;
            }
        }
    }

    return interaction;
}

void UI::Start(const RootDesc& desc) {
    state.arena.clear();
    state.nodes.clear();
    state.render_elements.clear();
    state.any_hovered = false;
    state.active_id = 0;
    state.next_id = 0;

    state.node_stack_size = 0;

    Node node;
    node.children = {};
    node.pos = glm::vec2(0.0f);
    node.size = desc.size();
    node.padding = desc.padding();
    node.sizing = UiSize::Fixed(desc.size().x, desc.size().y);
    node.gap = desc.gap();
    node.unique_id = state.next_id++;
    node.type_id = 0;
    node.orientation = desc.orientation();
    node.horizontal_alignment = desc.vertical_alignment();
    PushNode(std::move(node));
}

void UI::BeginElement(uint32_t type_id, const ElementDesc& desc, const void* custom_data_ptr, size_t custom_data_size) {
    const uint32_t id = state.next_id++;

    void* custom_data = nullptr;
    if (custom_data_ptr != nullptr && custom_data_size != 0) {
        custom_data = state.arena.allocate(custom_data_size);
        memcpy(custom_data, custom_data_ptr, custom_data_size);
    }

    glm::vec2 size = glm::vec2(0.0f);
    if (desc.size().width().type() == Sizing::Type::Fixed) {
        size.x = desc.size().width().value();
    }
    if (desc.size().height().type() == Sizing::Type::Fixed) {
        size.y = desc.size().height().value();
    }

    Node& top_node = TopNode();

    Node new_node;
    new_node.children = {};
    new_node.pos = glm::vec2(0.0f);
    new_node.size = size;
    new_node.padding = desc.padding();
    new_node.sizing = desc.size();
    new_node.custom_data = custom_data;
    new_node.custom_data_size = custom_data_size;
    new_node.gap = desc.gap();
    new_node.unique_id = id;
    new_node.type_id = type_id;
    new_node.orientation = desc.orientation();
    new_node.horizontal_alignment = desc.horizontal_alignment();
    new_node.vertical_alignment = desc.vertical_alignment();

    NodeAddElement(top_node, std::move(new_node));
}

void UI::EndElement() {
    SGE_ASSERT(state.node_stack_size > 0);

    Node& child = TopNode();
    --state.node_stack_size;

    Node& parent = TopNode();

    const float horizontal_padding = child.padding.left() + child.padding.right();
    const float vertical_padding = child.padding.top() + child.padding.bottom();

    if (parent.orientation == LayoutOrientation::Horizontal) {
        if (parent.sizing.width().type() != Sizing::Type::Fixed) {
            const float gap = (std::max<size_t>(child.children.size(), 1) - 1) * parent.gap;
            parent.size.x = horizontal_padding + gap;
        }

        for (uint32_t child_index : parent.children) {
            const Node& child = state.nodes[child_index];

            if (parent.sizing.width().type() != Sizing::Type::Fixed)
                parent.size.x += child.size.x;

            if (parent.sizing.height().type() != Sizing::Type::Fixed)
                parent.size.y = std::max(parent.size.y, child.size.y + vertical_padding);
        }

    } else if (parent.orientation == LayoutOrientation::Vertical) {
        if (parent.sizing.height().type() != Sizing::Type::Fixed) {
            const float gap = (std::max<size_t>(child.children.size(), 1) - 1) * parent.gap;
            parent.size.y = vertical_padding + gap;
        }
        for (uint32_t child_index : parent.children) {
            const Node& child = state.nodes[child_index];

            if (parent.sizing.width().type() != Sizing::Type::Fixed)
                parent.size.x = std::max(parent.size.x, child.size.x + horizontal_padding);

            if (parent.sizing.height().type() != Sizing::Type::Fixed)
                parent.size.y += child.size.y;
        }
    }
}

static void FinalizeLayout() {
    state.search_stack.clear();
    state.search_stack.push_back(&state.nodes[0]);

    while (!state.search_stack.empty()) {
        Node* current_node = state.search_stack.front();
        state.search_stack.pop_front();

        if (current_node->type_id != ReservedTypeID::Container) {
            state.render_elements.push_back(UiElement {
                .position = current_node->pos,
                .size = current_node->size,
                .custom_data = current_node->custom_data,
                .custom_data_size = current_node->custom_data_size,
                .unique_id = current_node->unique_id,
                .type_id = current_node->type_id,
                .z_index = current_node->z_index
            });
        }

        glm::vec2 pos = current_node->pos + glm::vec2(current_node->padding.left(), current_node->padding.top());

        for (uint32_t child_index : current_node->children) {
            Node& child = state.nodes[child_index];
            state.search_stack.push_back(&child);
            
            child.pos = pos;
            
            if (current_node->orientation == LayoutOrientation::Horizontal) {
                pos.x += child.size.x + current_node->gap;
            } else if (current_node->orientation == LayoutOrientation::Vertical) {
                pos.y += child.size.y + current_node->gap;
            }

            const float remaining_width = current_node->size.x - child.size.x;
            const float remaining_height = current_node->size.y - child.size.y;

            if (current_node->horizontal_alignment == Alignment::Center) {
                child.pos.x += remaining_width * 0.5f;
            } else if (current_node->horizontal_alignment == Alignment::End) {
                child.pos.x += remaining_width;
            }

            if (current_node->vertical_alignment == Alignment::Center) {
                child.pos.y += remaining_height * 0.5f;
            } else if (current_node->vertical_alignment == Alignment::End) {
                child.pos.y += remaining_height;
            }
        }
    }
}

const std::vector<UiElement>& UI::Finish() {
    FinalizeLayout();
    return state.render_elements;
};