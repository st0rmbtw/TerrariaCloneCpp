#include <cstddef>
#include <unordered_set>
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

        SGE_ASSERT(aligned_offset + size < m_capacity);

        void* ptr = m_data + aligned_offset;
        m_current_offset = aligned_offset + size;
        return ptr;
    }

    void reset() {
        m_current_offset = 0;
    }

private:
    uint8_t* m_data = nullptr;
    size_t m_current_offset = 0;
    size_t m_capacity = 0;
};

using NodeID = ElementID;

struct Node {
    NodeID unique_id{};
    std::function<void(sge::MouseButton)> on_press_callback = nullptr;

    std::vector<size_t> children;

    UiRect padding;
    UiSize sizing;

    glm::vec2 pos = glm::vec2(0.0f);
    glm::vec2 size = glm::vec2(0.0f);

    void* custom_data = nullptr;
    size_t custom_data_size = 0;

    float gap = 0.0f;
    
    uint32_t type_id = 0;

    uint32_t z_index = 0;
    
    LayoutOrientation orientation = LayoutOrientation::Horizontal;
    Alignment horizontal_alignment = Alignment::Start;
    Alignment vertical_alignment = Alignment::Start;
    bool render = false;
};

static struct {
    size_t node_stack[MAX_NODE_STACK_SIZE];
    
    std::unordered_set<NodeID> hovered_ids{};
    std::unordered_set<NodeID> search_visited{};
    sge::SwapbackVector<Node*> search_stack{};
    sge::SwapbackVector<Node*> postorder_stack{};
    
    Arena arena{ ARENA_SIZE };

    std::vector<Node> nodes{};
    std::vector<UiElement> render_elements{};
    
    size_t node_stack_size = 0;

    NodeID active_id{};

    bool any_hovered = false;
} state;

static Node& TopNode() {
    SGE_ASSERT(state.node_stack_size > 0);

    const size_t index = state.node_stack[state.node_stack_size-1];
    return state.nodes[index];
}

static Node& ParentNode() {
    SGE_ASSERT(state.node_stack_size > 1);
    const size_t index = state.node_stack[state.node_stack_size-2];
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

static bool CheckIfPressed(NodeID id, const sge::Rect element_rect, sge::MouseButton button) {
    const bool hovered = element_rect.contains(sge::Input::MouseScreenPosition());

    if (state.active_id.id == 0) {
        if (hovered && sge::Input::JustPressed(button)) {
            state.active_id = id;
        }
    } else if (state.active_id == id) {
        if (sge::Input::JustReleased(button)) {
            state.active_id = NodeID();
            if (hovered) {
                return true;
            }
        }
    }

    return false;
}

void UI::Update() {
    state.hovered_ids.clear();
    state.any_hovered = false;

    if (state.nodes.empty())
        return;

    state.search_stack.clear();
    state.search_stack.push_back(&state.nodes[0]);

    state.search_visited.clear();
    state.search_visited.insert(state.nodes[0].unique_id);

    state.postorder_stack.clear();

    while (!state.search_stack.empty()) {
        Node* current_node = state.search_stack.back();
        state.search_stack.pop_back();

        const sge::Rect element_rect = sge::Rect::from_top_left(current_node->pos, current_node->size);
        const bool hovered = element_rect.contains(sge::Input::MouseScreenPosition());

        if (hovered) {
            state.hovered_ids.insert(current_node->unique_id);
            state.any_hovered = true;
        }

        const bool clickable = current_node->on_press_callback != nullptr;
        if (clickable) {
            state.postorder_stack.push_back(current_node);
        }

        for (uint32_t child_index : current_node->children) {
            Node& child = state.nodes[child_index];

            if (state.search_visited.contains(child.unique_id)) {
                continue;
            }

            state.search_stack.push_back(&child);
            state.search_visited.insert(child.unique_id);
        }
    }

    while (!state.postorder_stack.empty()) {
        Node* node = state.postorder_stack.back();
        state.postorder_stack.pop_back();

        const sge::Rect element_rect = sge::Rect::from_top_left(node->pos, node->size);

        if (CheckIfPressed(node->unique_id, element_rect, sge::MouseButton::Left)) {
            node->on_press_callback(sge::MouseButton::Left);
            break;
        }

        if (CheckIfPressed(node->unique_id, element_rect, sge::MouseButton::Right)) {
            node->on_press_callback(sge::MouseButton::Right);
            break;
        }

        if (CheckIfPressed(node->unique_id, element_rect, sge::MouseButton::Middle)) {
            node->on_press_callback(sge::MouseButton::Middle);
            break;
        }
    }
}

void UI::Start(const RootDesc& desc) {
    state.arena.reset();
    state.nodes.clear();
    state.render_elements.clear();

    state.node_stack_size = 0;

    // Create root node
    Node node;
    {
        node.children = {};
        node.pos = glm::vec2(0.0f);
        node.size = desc.size();
        node.padding = desc.padding();
        node.sizing = UiSize::Fixed(desc.size().x, desc.size().y);
        node.gap = desc.gap();
        node.unique_id = NodeID();
        node.type_id = 0;
        node.orientation = desc.orientation();
        node.horizontal_alignment = desc.vertical_alignment();
        node.render = false;
    }
    PushNode(std::move(node));
}

static ElementID GenerateId(Node& parent) {
    ElementID id = HashNumber(parent.children.size(), parent.unique_id.id);
    return id;
}

void UI::BeginElement(uint32_t type_id, const ElementDesc& desc, bool render) {
    glm::vec2 size = glm::vec2(0.0f);
    if (desc.size().width().type() == Sizing::Type::Fixed) {
        size.x = desc.size().width().value();
    }
    if (desc.size().height().type() == Sizing::Type::Fixed) {
        size.y = desc.size().height().value();
    }

    Node& parent = TopNode();

    NodeID id = desc.id();
    if (id.id == 0) {
        id = GenerateId(parent);
    }

    Node new_node;
    {
        new_node.children = {};
        new_node.pos = glm::vec2(0.0f);
        new_node.size = size;
        new_node.padding = desc.padding();
        new_node.sizing = desc.size();
        new_node.custom_data = nullptr;
        new_node.custom_data_size = 0;
        new_node.gap = desc.gap();
        new_node.unique_id = id;
        new_node.type_id = type_id;
        new_node.orientation = desc.orientation();
        new_node.horizontal_alignment = desc.horizontal_alignment();
        new_node.vertical_alignment = desc.vertical_alignment();
        new_node.render = render;
    }
    NodeAddElement(parent, std::move(new_node));
}

void UI::EndElement() {
    SGE_ASSERT(state.node_stack_size > 0);

    Node& node = TopNode();
    --state.node_stack_size;

    const float horizontal_padding = node.padding.left() + node.padding.right();
    const float vertical_padding = node.padding.top() + node.padding.bottom();

    if (node.orientation == LayoutOrientation::Horizontal) {
        if (node.sizing.width().type() != Sizing::Type::Fixed) {
            const float gap = (std::max<size_t>(node.children.size(), 1) - 1) * node.gap;
            node.size.x += horizontal_padding + gap;
        }

        for (uint32_t child_index : node.children) {
            const Node& child = state.nodes[child_index];

            if (node.sizing.width().type() != Sizing::Type::Fixed)
                node.size.x += child.size.x;

            if (node.sizing.height().type() != Sizing::Type::Fixed)
                node.size.y = std::max(node.size.y, child.size.y + vertical_padding);
        }

    } else if (node.orientation == LayoutOrientation::Vertical) {
        if (node.sizing.height().type() != Sizing::Type::Fixed) {
            const float gap = (std::max<size_t>(node.children.size(), 1) - 1) * node.gap;
            node.size.y += vertical_padding + gap;
        }
        for (uint32_t child_index : node.children) {
            const Node& child = state.nodes[child_index];

            if (node.sizing.width().type() != Sizing::Type::Fixed)
                node.size.x = std::max(node.size.x, child.size.x + horizontal_padding);

            if (node.sizing.height().type() != Sizing::Type::Fixed)
                node.size.y += child.size.y;
        }
    }
}

static void FinalizeLayout() {
    state.search_stack.clear();
    state.search_stack.push_back(&state.nodes[0]);

    state.search_visited.clear();
    state.search_visited.insert(state.nodes[0].unique_id);

    while (!state.search_stack.empty()) {
        Node* current_node = state.search_stack.front();
        state.search_stack.pop_front();

        if (current_node->render) {
            state.render_elements.push_back(UiElement {
                .position = current_node->pos,
                .size = current_node->size,
                .unique_id = current_node->unique_id,
                .custom_data = current_node->custom_data,
                .custom_data_size = current_node->custom_data_size,
                .type_id = current_node->type_id,
                .z_index = current_node->z_index
            });
        }

        glm::vec2 pos = current_node->pos + glm::vec2(current_node->padding.left(), current_node->padding.top());

        for (uint32_t child_index : current_node->children) {
            Node& child = state.nodes[child_index];

            if (state.search_visited.contains(child.unique_id)) {
                continue;
            }

            state.search_stack.push_back(&child);
            state.search_visited.insert(child.unique_id);
            
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

void UI::SetCustomData(const void* custom_data_ptr, size_t custom_data_size, size_t custom_data_alignment) {
    Node& node = TopNode();

    void* custom_data = nullptr;
    if (custom_data_ptr != nullptr && custom_data_size > 0 && custom_data_alignment > 0) {
        custom_data = state.arena.allocate(custom_data_size, custom_data_alignment);
        memcpy(custom_data, custom_data_ptr, custom_data_size);
    }

    node.custom_data = custom_data;
    node.custom_data_size = custom_data_size;
}

void UI::OnClick(std::function<void(sge::MouseButton)>&& on_press) {
    Node& node = TopNode();
    node.on_press_callback = std::move(on_press);
}

uint32_t UI::GetParentID() {
    const Node& node = ParentNode();
    return node.unique_id.id;
}

const ElementID& UI::GetElementID() {
    const Node& node = TopNode();
    return node.unique_id;
}

bool UI::IsHovered() {
    const Node& node = TopNode();
    return state.hovered_ids.contains(node.unique_id);
}

const std::vector<UiElement>& UI::Finish() {
    FinalizeLayout();
    return state.render_elements;
};