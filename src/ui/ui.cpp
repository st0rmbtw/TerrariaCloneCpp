#include <cstddef>
#include <deque>
#include <unordered_set>
#include <vector>

#include <SGE/assert.hpp>
#include <SGE/input.hpp>
#include <SGE/math/rect.hpp>
#include <SGE/utils/containers/swapbackvector.hpp>
#include <SGE/types/rich_text.hpp>
#include <SGE/utils/text.hpp>
#include <SGE/profile.hpp>
#include <SGE/time/time.hpp>

#include "arena.hpp"
#include "ui.hpp"

inline constexpr size_t MAX_NODE_STACK_SIZE = 100;
inline constexpr size_t ARENA_CAPACITY = 1024 * 1024;

using NodeID = ElementID;

template <typename T>
class ArenaAllocator {
public:
    using value_type = T;

    ArenaAllocator(Arena& arena) : m_arena{ &arena } {}

    value_type* allocate(std::size_t n) {
        return m_arena->allocate<value_type>(n);
    }

    void deallocate(value_type*, std::size_t) {
        // do nothing
    }

private:
    Arena* m_arena = nullptr;
};

struct Node {
    NodeID unique_id{};
    std::function<void(sge::MouseButton)> on_click_callback = nullptr;

    std::vector<uint32_t, ArenaAllocator<uint32_t>> children;
    
    UiRect padding;
    UiSize sizing;
    
    glm::vec2 pos = glm::vec2(0.0f);
    glm::vec2 size = glm::vec2(0.0f);
    glm::vec2 offset = glm::vec2(0.0f);
    
    void* custom_data = nullptr;
    size_t custom_data_size = 0;
    
    float gap = 0.0f;
    float scroll_max = 0.0f;
    
    uint32_t type_id = 0;
    uint32_t z_index = 0;
    uint32_t text_data_index = 0;
    
    LayoutOrientation orientation = LayoutOrientation::Horizontal;
    std::optional<Alignment> self_alignment = Alignment::Start;
    Alignment horizontal_alignment = Alignment::Start;
    Alignment vertical_alignment = Alignment::Start;

    bool render = false;
    bool text_node = false;
    bool hoverable = false;
    bool scrollable = false;
    bool scissor_start = false;
    bool scissor_end = false;

    Node(Arena& arena) : children{ arena } {}
};

struct ScrollData {
    float offset;
};

enum class SearchState : uint8_t {
    PreProcess = 0,
    PostProcess,
    Done
};

struct SearchStackFrame {
    Node* parent;
    uint32_t child_pos;
    SearchState state;
};

static struct {
    uint32_t node_stack[MAX_NODE_STACK_SIZE];
    
    std::unordered_set<NodeID> hovered_ids{};
    std::unordered_set<NodeID> search_visited{};

    NodeID active_id{};

    std::deque<Node*> search_stack{};
    std::deque<SearchStackFrame> search_stack_frame{};
    sge::SwapbackVector<Node*> postorder_stack{};

    sge::SwapbackVector<Node*> growable_nodes{};
    
    Arena arena{ ARENA_CAPACITY };

    std::vector<Node> nodes{};
    std::vector<TextData> text_data{};
    std::unordered_map<NodeID, ScrollData> scroll_data{};
    std::vector<UiElement> render_elements{};
    
    size_t node_stack_size = 0;

    bool any_hovered = false;
} state;

[[nodiscard]]
static inline bool NodeIsClickable(const Node& node) noexcept {
    return node.on_click_callback != nullptr;
}

static Node& TopNode() noexcept {
    SGE_ASSERT(state.node_stack_size > 0);
    const uint32_t index = state.node_stack[state.node_stack_size-1];
    return state.nodes[index];
}

static Node& ParentNode() noexcept {
    SGE_ASSERT(state.node_stack_size > 1);
    const uint32_t index = state.node_stack[state.node_stack_size-2];
    return state.nodes[index];
}

static void NodeAddElement(Node& parent, Node&& child) {
    SGE_ASSERT(!parent.text_node);
    SGE_ASSERT(state.nodes.size() < UINT32_MAX);
    SGE_ASSERT(state.node_stack_size < MAX_NODE_STACK_SIZE);

    const uint32_t index = state.nodes.size();
    parent.children.push_back(index);
    state.nodes.push_back(std::move(child));
}

static void PushNode(Node& parent, Node&& child) {
    SGE_ASSERT(!parent.text_node);
    SGE_ASSERT(state.nodes.size() < UINT32_MAX);
    SGE_ASSERT(state.node_stack_size < MAX_NODE_STACK_SIZE);

    const uint32_t index = state.nodes.size();
    parent.children.push_back(index);
    
    state.node_stack[state.node_stack_size] = index;
    state.node_stack_size += 1;

    state.nodes.push_back(std::move(child));
}

static void PushNode(Node&& node) {
    SGE_ASSERT(state.nodes.size() < UINT32_MAX);
    SGE_ASSERT(state.node_stack_size < MAX_NODE_STACK_SIZE);

    const uint32_t index = state.nodes.size();
    state.nodes.push_back(std::move(node));

    state.node_stack[state.node_stack_size] = index;
    state.node_stack_size += 1;
}

static inline ElementID HashNumber(const uint32_t offset, const uint32_t seed) noexcept {
    uint32_t hash = seed;
    hash += (offset + 48);
    hash += (hash << 10);
    hash ^= (hash >> 6);

    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    return ElementID {
        .string_id = {},
        .id = hash + 1, // +1 because the element with the id of 0 is the root element
        .offset = offset,
        .base_id = seed
    };
}

static inline ElementID HashString(const std::string_view key, const uint32_t seed) noexcept {
    uint32_t hash = seed;

    for (size_t i = 0; i < key.size(); i++) {
        hash += key.data()[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }

    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    // +1 because the element with the id of 0 is the root element
    return ElementID {
        .string_id = key,
        .id = hash + 1,
        .offset = 0,
        .base_id = hash + 1,
    };
}

static inline ElementID HashStringWithOffset(const std::string_view key, const uint32_t offset, const uint32_t seed) {
    uint32_t hash = 0;
    uint32_t base = seed;

    for (size_t i = 0; i < key.size(); i++) {
        base += key.data()[i];
        base += (base << 10);
        base ^= (base >> 6);
    }
    hash = base;
    hash += offset;
    hash += (hash << 10);
    hash ^= (hash >> 6);

    hash += (hash << 3);
    base += (base << 3);
    hash ^= (hash >> 11);
    base ^= (base >> 11);
    hash += (hash << 15);
    base += (base << 15);

    // +1 because the element with the id of 0 is the root element
    return ElementID {
        .string_id = key,
        .id = hash + 1,
        .offset = offset,
        .base_id = base + 1,
    };
}

static inline std::string_view CopyStringToArena(std::string_view string) {
    using pointer = std::string_view::pointer;
    using value_type = std::string_view::value_type;
    pointer str = static_cast<pointer>(state.arena.allocate(string.size() + 1, alignof(value_type)));
    memcpy(str, string.data(), string.size());
    str[string.size()] = '\0'; // Add a null terminator just in case
    return { str, string.size() + 1 };
}

static bool CheckIfPressed(const NodeID id, const sge::Rect element_rect, const sge::MouseButton button) noexcept {
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

    state.postorder_stack.clear();

    while (!state.search_stack.empty()) {
        Node* current_node = state.search_stack.back();
        state.search_stack.pop_back();

        if (state.search_visited.contains(current_node->unique_id)) {
            continue;
        }

        state.search_visited.insert(current_node->unique_id);

        if (current_node->text_node)
            continue;

        const sge::Rect element_rect = sge::Rect::from_top_left(current_node->pos, current_node->size);
        const bool hovered = element_rect.contains(sge::Input::MouseScreenPosition());

        const bool clickable = NodeIsClickable(*current_node);

        if (hovered) {
            state.hovered_ids.insert(current_node->unique_id);
            state.any_hovered = state.any_hovered || clickable || current_node->hoverable;

            if (current_node->scrollable) {
                ScrollData& scroll_data = state.scroll_data[current_node->unique_id];
                for (float delta : sge::Input::ScrollEvents()) {
                    scroll_data.offset += delta * 1000.0f * sge::Time::DeltaSeconds();
                    scroll_data.offset = std::clamp(scroll_data.offset, -current_node->scroll_max, 0.0f);
                }
            }
        }

        if (clickable) {
            state.postorder_stack.push_back(current_node);
        }

        for (uint32_t child_index : current_node->children) {
            Node& child = state.nodes[child_index];
            state.search_stack.push_back(&child);
        }
    }

    while (!state.postorder_stack.empty()) {
        Node* node = state.postorder_stack.back();
        state.postorder_stack.pop_back();

        const sge::Rect element_rect = sge::Rect::from_top_left(node->pos, node->size);

        if (CheckIfPressed(node->unique_id, element_rect, sge::MouseButton::Left)) {
            node->on_click_callback(sge::MouseButton::Left);
            break;
        }

        if (CheckIfPressed(node->unique_id, element_rect, sge::MouseButton::Right)) {
            node->on_click_callback(sge::MouseButton::Right);
            break;
        }

        if (CheckIfPressed(node->unique_id, element_rect, sge::MouseButton::Middle)) {
            node->on_click_callback(sge::MouseButton::Middle);
            break;
        }
    }
}

void UI::Start(const RootDesc& desc) {
    ZoneScoped;

    state.arena.clear();
    state.nodes.clear();
    state.render_elements.clear();
    state.text_data.clear();

    state.node_stack_size = 0;

    // Create root node
    Node node(state.arena);
    {
        node.unique_id = NodeID();
        node.size = desc.size();
        node.padding = desc.padding();
        node.sizing = UiSize::Fixed(desc.size().x, desc.size().y);
        node.gap = desc.gap();
        node.orientation = desc.orientation();
        node.horizontal_alignment = desc.horizontal_alignment();
        node.vertical_alignment = desc.vertical_alignment();
    }
    PushNode(std::move(node));
}

static inline ElementID GenerateId(Node& parent) noexcept {
    SGE_ASSERT(!parent.text_node);
    ElementID id = HashNumber(parent.children.size(), parent.unique_id.id);
    return id;
}

void UI::BeginElement(uint32_t type_id, const ElementDesc& desc, bool render) {
    ZoneScoped;

    glm::vec2 size = glm::vec2(0.0f);
    if (desc.size.width().type() == Sizing::Type::Fixed) {
        size.x = desc.size.width().value();
    }
    if (desc.size.height().type() == Sizing::Type::Fixed) {
        size.y = desc.size.height().value();
    }

    Node& parent = TopNode();

    NodeID id = desc.id;
    if (id.id == 0) {
        id = GenerateId(parent);
    }

    Node new_node(state.arena);
    {
        new_node.unique_id = id;
        new_node.size = size;
        new_node.offset = desc.offset;
        new_node.sizing = desc.size;
        new_node.padding = desc.padding;
        new_node.orientation = desc.orientation;
        new_node.horizontal_alignment = desc.horizontal_alignment;
        new_node.vertical_alignment = desc.vertical_alignment;
        new_node.self_alignment = desc.self_alignment;
        new_node.gap = desc.gap;
        new_node.hoverable = desc.hoverable;
        new_node.scrollable = desc.scrollable;
        new_node.type_id = type_id;
        new_node.z_index = parent.z_index + 1;
        new_node.render = render;
    }

    if (desc.scrollable && !state.scroll_data.contains(id)) {
        state.scroll_data[id] = ScrollData{};
    }

    PushNode(parent, std::move(new_node));
}

static void GrowElementsHorizontally(Node& parent) {
    ZoneScoped;

    if (parent.text_node) return;
    if (parent.children.empty()) return;

    float remaining_width = parent.size.x - parent.padding.left() - parent.padding.right();

    if (parent.orientation == LayoutOrientation::Horizontal) {
        for (uint32_t child_index : parent.children) {
            Node& child = state.nodes[child_index];
            remaining_width -= child.size.x;
        }
        remaining_width -= (parent.children.size() - 1) * parent.gap;
    } 

    state.growable_nodes.clear();

    for (uint32_t child_index : parent.children) {
        Node& child = state.nodes[child_index];
        if (child.sizing.width().type() == Sizing::Type::Fill) {
            state.growable_nodes.push_back(&child);
        }
    }

    if (state.growable_nodes.empty())
        return;

    while (remaining_width > 0) {
        float smallest = state.growable_nodes[0]->size.x;
        float second_smallest = INFINITY;
        float width_to_add = remaining_width;
        for (Node* child : state.growable_nodes) {
            if (child->size.x < smallest) {
                second_smallest = smallest;
                smallest = child->size.x;
            }

            if (child->size.x > smallest) {
                second_smallest = std::min(second_smallest, child->size.x);
                width_to_add = second_smallest - smallest;
            }
        }

        if (parent.orientation == LayoutOrientation::Horizontal) {
            width_to_add = std::min(width_to_add, remaining_width / state.growable_nodes.size());
        }

        for (Node* child : state.growable_nodes) {
            if (sge::approx_equals(child->size.x, smallest)) {
                child->size.x += width_to_add;
                remaining_width -= width_to_add;
            }
        }
    }
}

static void GrowElementsVertically(Node& parent) {
    ZoneScoped;

    if (parent.text_node) return;
    if (parent.children.empty()) return;

    float remaining_height = parent.size.y - parent.padding.top() - parent.padding.bottom();

    if (parent.orientation == LayoutOrientation::Vertical) {
        for (uint32_t child_index : parent.children) {
            Node& child = state.nodes[child_index];
            remaining_height -= child.size.y;
        }
        remaining_height -= (parent.children.size() - 1) * parent.gap;
    }

    state.growable_nodes.clear();

    for (uint32_t child_index : parent.children) {
        Node& child = state.nodes[child_index];
        if (child.sizing.height().type() == Sizing::Type::Fill) {
            state.growable_nodes.push_back(&child);
        }
    }

    if (state.growable_nodes.empty())
        return;
    
    while (remaining_height > 0) {
        float smallest = state.growable_nodes[0]->size.y;
        float second_smallest = INFINITY;
        float height_to_add = remaining_height;
        for (Node* child : state.growable_nodes) {
            if (child->size.y < smallest) {
                second_smallest = smallest;
                smallest = child->size.y;
            }

            if (child->size.y > smallest) {
                second_smallest = std::min(second_smallest, child->size.y);
                height_to_add = second_smallest - smallest;
            }
        }

        if (parent.orientation == LayoutOrientation::Vertical) {
            height_to_add = std::min(height_to_add, remaining_height / state.growable_nodes.size());
        }

        for (Node* child : state.growable_nodes) {
            if (sge::approx_equals(child->size.y, smallest)) {
                child->size.y += height_to_add;
                remaining_height -= height_to_add;
            }
        }
    }
}

static inline Node& GetChild(Node& node, uint32_t index) {
    const uint32_t idx = node.children[index];
    return state.nodes[idx];
}

static uint32_t FindMaxZIndex(Node& node) {
    uint32_t z_index = node.z_index;

    state.search_stack.clear();
    state.search_stack.push_back(&node);

    while (!state.search_stack.empty()) {
        Node* current_node = state.search_stack.back();
        state.search_stack.pop_back();

        z_index = std::max(z_index, current_node->z_index);
        
        for (uint32_t child_index : current_node->children) {
            Node& child = state.nodes[child_index];
            state.search_stack.push_back(&child);
        }
    }

    return z_index;
}

static void PropagateZIndex(Node& node, uint32_t z_index) {
    node.z_index = z_index;
    
    if (node.orientation == LayoutOrientation::Stack && node.children.size() > 1) {
        uint32_t i = 0;
        do {
            uint32_t z_index = std::max(node.z_index, FindMaxZIndex(GetChild(node, i)));
            Node& child = state.nodes[node.children[i + 1]];
            PropagateZIndex(child, ++z_index);
        } while (++i < node.children.size() - 1);
    } else {
        for (uint32_t child_index : node.children) {
            Node& child = state.nodes[child_index];
            PropagateZIndex(child, z_index + 1);
        }
    }
}

void UI::EndElement() {
    ZoneScoped;

    SGE_ASSERT(state.node_stack_size > 0);

    Node& node = TopNode();
    --state.node_stack_size;

    if (node.text_node) return;

    PropagateZIndex(node, node.z_index);

    const size_t children_count = node.children.size();
    if (node.scrollable && children_count > 0) {
        uint32_t i = 0;

        for (uint32_t child_index : node.children) {
            Node& child = state.nodes[child_index];

            if (i == 0) {
                child.scissor_start = true;
            }

            if (i == children_count - 1) {
                child.scissor_end = true;
            }

            ++i;
        }
    }

    const float horizontal_padding = node.padding.left() + node.padding.right();
    const float vertical_padding = node.padding.top() + node.padding.bottom();

    if (node.sizing.width().type() == Sizing::Type::Fit) {
        node.size.x += horizontal_padding;
    }
    if (node.sizing.height().type() == Sizing::Type::Fit) {
        node.size.y += vertical_padding;
    }

    if (node.orientation == LayoutOrientation::Horizontal) {
        if (node.sizing.width().type() == Sizing::Type::Fit) {
            const float gap = (std::max<size_t>(node.children.size(), 1) - 1) * node.gap;
            node.size.x += gap;
        }

        for (uint32_t child_index : node.children) {
            const Node& child = state.nodes[child_index];

            if (node.sizing.width().type() == Sizing::Type::Fit)
                node.size.x += child.size.x;

            if (node.sizing.height().type() == Sizing::Type::Fit)
                node.size.y = std::max(node.size.y, child.size.y + vertical_padding);
        }
    } else if (node.orientation == LayoutOrientation::Vertical) {
        if (node.sizing.height().type() == Sizing::Type::Fit) {
            const float gap = (std::max<size_t>(node.children.size(), 1) - 1) * node.gap;
            node.size.y += gap;
        }
        for (uint32_t child_index : node.children) {
            const Node& child = state.nodes[child_index];

            if (node.sizing.width().type() == Sizing::Type::Fit)
                node.size.x = std::max(node.size.x, child.size.x + horizontal_padding);

            if (node.sizing.height().type() == Sizing::Type::Fit)
                node.size.y += child.size.y;
        }
    } else if (node.orientation == LayoutOrientation::Stack) {
        for (uint32_t child_index : node.children) {
            const Node& child = state.nodes[child_index];

            if (node.sizing.width().type() == Sizing::Type::Fit)
                node.size.x = std::max(node.size.x, child.size.x + horizontal_padding);

            if (node.sizing.height().type() == Sizing::Type::Fit)
                node.size.y = std::max(node.size.y, child.size.y + vertical_padding);
        }
    }
}

void UI::Text(uint32_t type_id, const sge::Font& font, const sge::RichTextSection* sections, const size_t count, const TextElementDesc& desc) {
    ZoneScoped;

    const glm::vec2 measured_size = sge::calculate_text_bounds(font, sections, count);

    // Copy section contents
    sge::RichTextSection* arena_sections = state.arena.allocate<sge::RichTextSection>(count);
    for (size_t i = 0; i < count; ++i) {
        const sge::RichTextSection& section = sections[i];
        arena_sections[i].size = section.size;
        arena_sections[i].color = section.color;
        arena_sections[i].text = CopyStringToArena(section.text);
    }

    const size_t text_data_index = state.text_data.size();
    state.text_data.push_back(TextData {
        .font = font,
        .sections = arena_sections,
        .sections_count = count
    });

    Node& parent = TopNode();

    NodeID id = desc.id;
    if (id.id == 0) {
        id = GenerateId(parent);
    }

    Node new_node(state.arena);
    {
        new_node.unique_id = id;
        new_node.size = measured_size;
        new_node.offset = desc.offset;
        new_node.sizing = UiSize::Fixed(measured_size);
        new_node.text_data_index = text_data_index;
        new_node.type_id = type_id;
        new_node.z_index = parent.z_index + 1;
        new_node.render = true;
        new_node.text_node = true;
        new_node.self_alignment = desc.self_alignment;
    }
    NodeAddElement(parent, std::move(new_node));
}

static void FinalizeLayout() {
    ZoneScoped;

    state.search_stack.clear();
    state.search_stack.push_back(&state.nodes[0]);

    state.search_visited.clear();
    state.search_visited.insert(state.nodes[0].unique_id);

    while (!state.search_stack.empty()) {
        Node* current_node = state.search_stack.front();
        state.search_stack.pop_front();

        GrowElementsHorizontally(*current_node);
        GrowElementsVertically(*current_node);

        if (current_node->text_node)
            continue;

        const LayoutOrientation parent_orientation = current_node->orientation;
        const glm::vec2 parent_size = current_node->size;
        const UiRect parent_padding = current_node->padding;
        const Alignment parent_horizontal_alignment = current_node->horizontal_alignment;
        const Alignment parent_vertical_alignment = current_node->vertical_alignment;
        const float parent_gap = current_node->gap;

        glm::vec2 pos = current_node->pos + current_node->offset + glm::vec2(parent_padding.left(), parent_padding.top());

        if (current_node->scrollable) {
            current_node->scroll_max = current_node->scroll_max;

            ScrollData& scroll_data = state.scroll_data[current_node->unique_id];
            if (parent_orientation == LayoutOrientation::Vertical) {
                pos.y += scroll_data.offset;
            } else if (parent_orientation == LayoutOrientation::Horizontal) {
                pos.x += scroll_data.offset;
            }
        }

        float max_scroll = 0.0f;

        float remaining_width = parent_size.x - parent_padding.left() - parent_padding.right();
        float remaining_height = parent_size.y - parent_padding.top() - parent_padding.bottom();

        for (uint32_t child_index : current_node->children) {
            Node& child = state.nodes[child_index];
            
            if (state.search_visited.contains(child.unique_id)) {
                continue;
            }

            child.pos = pos;
            
            state.search_stack.push_back(&child);
            state.search_visited.insert(child.unique_id);
            
            if (parent_orientation == LayoutOrientation::Horizontal) {
                pos.x += child.size.x + parent_gap;
                remaining_height = parent_size.y - child.size.y - parent_padding.top() - parent_padding.bottom();
            } else if (parent_orientation == LayoutOrientation::Vertical) {
                pos.y += child.size.y + parent_gap;
                remaining_width = parent_size.x - child.size.x - parent_padding.left() - parent_padding.right();
            } else if (parent_orientation == LayoutOrientation::Stack) {
                remaining_width = parent_size.x - child.size.x - parent_padding.left() - parent_padding.right();
                remaining_height = parent_size.y - child.size.y - parent_padding.top() - parent_padding.bottom();
            }

            if (parent_orientation == LayoutOrientation::Horizontal) {
                if (child.self_alignment) {
                    if (child.self_alignment == Alignment::Center) {
                        child.pos.y += remaining_height * 0.5f;
                    } else if (child.self_alignment == Alignment::End) {
                        child.pos.y += remaining_height;
                    }
                } else {
                    if (parent_vertical_alignment == Alignment::Center) {
                        child.pos.y += remaining_height * 0.5f;
                    } else if (parent_vertical_alignment == Alignment::End) {
                        child.pos.y += remaining_height;
                    }
                }

                remaining_width -= child.size.x;
                max_scroll += child.size.x + parent_gap;
            } else if (parent_orientation == LayoutOrientation::Vertical) {
                if (child.self_alignment) {
                    if (child.self_alignment == Alignment::Center) {
                        child.pos.x += remaining_width * 0.5f;
                    } else if (child.self_alignment == Alignment::End) {
                        child.pos.x += remaining_width;
                    }
                } else {
                    if (parent_horizontal_alignment == Alignment::Center) {
                        child.pos.x += remaining_width * 0.5f;
                    } else if (parent_horizontal_alignment == Alignment::End) {
                        child.pos.x += remaining_width;
                    }
                }

                remaining_height -= child.size.y;
                max_scroll += child.size.y + parent_gap;
            } else if (parent_orientation == LayoutOrientation::Stack) {
                if (child.self_alignment) {
                    switch (child.self_alignment.value()) {
                    case Alignment::Start:
                    case Alignment::TopLeft:
                        // At top left by default
                    break;
                    case Alignment::TopCenter:
                        child.pos.x += remaining_width * 0.5f;
                    break;
                    case Alignment::End:
                    case Alignment::TopRight:
                        child.pos.x += remaining_width;
                    break;
                    case Alignment::CenterLeft:
                        child.pos.y += remaining_height * 0.5f;
                    break;
                    case Alignment::Center:
                        child.pos.x += remaining_width * 0.5f;
                        child.pos.y += remaining_height * 0.5f;
                    break;
                    case Alignment::CenterRight:
                        child.pos.x += remaining_width;
                        child.pos.y += remaining_height * 0.5f;
                    break;
                    case Alignment::BottomLeft:
                        child.pos.y += remaining_height;
                    break;
                    case Alignment::BottomCenter:
                        child.pos.x += remaining_width * 0.5f;
                        child.pos.y += remaining_height;
                    break;
                    case Alignment::BottomRight:
                        child.pos.x += remaining_width;
                        child.pos.y += remaining_height;
                    break;
                    }
                } else {
                    if (parent_horizontal_alignment == Alignment::Center) {
                        child.pos.x += remaining_width * 0.5f;
                    } else if (parent_horizontal_alignment == Alignment::End) {
                        child.pos.x += remaining_width;
                    }

                    if (parent_vertical_alignment == Alignment::Center) {
                        child.pos.y += remaining_height * 0.5f;
                    } else if (parent_vertical_alignment == Alignment::End) {
                        child.pos.y += remaining_height;
                    }
                }
            }
        }

        if (parent_orientation == LayoutOrientation::Horizontal) {
            current_node->scroll_max = max_scroll - parent_size.x + parent_padding.right();
        } else if (parent_orientation == LayoutOrientation::Vertical) {
            current_node->scroll_max = max_scroll - parent_size.y + parent_padding.bottom();
        }
    }

    state.search_stack_frame.clear();
    state.search_stack_frame.push_back(SearchStackFrame{ &state.nodes[0], 0, SearchState::Done });

    while (!state.search_stack_frame.empty()) {
        SearchStackFrame& frame = state.search_stack_frame.back();

        const Node& parent = *frame.parent;
        const sge::Rect parent_rect = sge::Rect::from_top_left(parent.pos, parent.size);

        switch (frame.state) {
        case SearchState::Done: {
            if (frame.child_pos >= parent.children.size()) {
                state.search_stack_frame.pop_back();
                continue;
            }

            frame.state = SearchState::PreProcess;
        } break;
        case SearchState::PreProcess: {
            const uint32_t child_index = parent.children[frame.child_pos];
            Node& child = state.nodes[child_index];

            const sge::Rect child_rect = sge::Rect::from_top_left(child.pos, child.size);

            if (child.scrollable) {
                ScissorData* scissor_data = nullptr;
                scissor_data = state.arena.allocate<ScissorData>();
                scissor_data->area = sge::IRect::from_top_left(child.pos, glm::round(child.size));

                state.render_elements.push_back(UiElement {
                    .scissor_data = scissor_data,
                    .scissor_start = true,
                    .scissor_end = false
                });
            }

            if (child.render && parent_rect.intersects(child_rect)) {
                TextData* text_data = nullptr;
                if (child.text_node) {
                    text_data = &state.text_data[child.text_data_index];
                }

                state.render_elements.push_back(UiElement {
                    .unique_id = child.unique_id,
                    .position = child.pos + child.offset,
                    .size = child.size,
                    .custom_data = child.custom_data,
                    .custom_data_size = child.custom_data_size,
                    .text_data = text_data,
                    .type_id = child.type_id,
                    .z_index = child.z_index,
                    .scissor_start = false,
                    .scissor_end = false
                });
            }

            state.search_stack_frame.push_back(SearchStackFrame{ &child, 0, SearchState::Done });

            frame.state = SearchState::PostProcess;
        } break;
        case SearchState::PostProcess: {
            const uint32_t child_index = parent.children[frame.child_pos];
            Node& child = state.nodes[child_index];

            if (child.scrollable) {
                state.render_elements.push_back(UiElement {
                    .scissor_start = false,
                    .scissor_end = true
                });
            }

            frame.child_pos++;
            frame.state = SearchState::Done;
        } break;
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
    node.on_click_callback = std::move(on_press);
}

uint32_t UI::GetParentID() noexcept {
    const Node& node = ParentNode();
    return node.unique_id.id;
}

const ElementID& UI::GetElementID() noexcept {
    const Node& node = TopNode();
    return node.unique_id;
}

bool UI::IsHovered() noexcept {
    const Node& node = TopNode();
    return state.hovered_ids.contains(node.unique_id);
}

bool UI::IsMouseOverUi() noexcept {
    return state.any_hovered;
}

const std::vector<UiElement>& UI::Finish() {
    FinalizeLayout();
    return state.render_elements;
};

ElementID ID::Local(const std::string_view key) noexcept {
    const uint32_t parent_id = TopNode().unique_id.id;
    return HashString(CopyStringToArena(key), parent_id);
}

ElementID ID::Local(const std::string_view key, uint32_t index) noexcept {
    const uint32_t parent_id = TopNode().unique_id.id;
    return HashStringWithOffset(CopyStringToArena(key), index, parent_id);
}

ElementID ID::Global(const std::string_view key, uint32_t index) noexcept {
    CopyStringToArena(key);
    return HashString(CopyStringToArena(key), index);
}