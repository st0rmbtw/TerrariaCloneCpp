#include <cstddef>
#include <unordered_set>
#include <vector>
#include <new>

#include <SGE/assert.hpp>
#include <SGE/input.hpp>
#include <SGE/math/rect.hpp>
#include <SGE/utils/containers/swapbackvector.hpp>
#include <SGE/types/rich_text.hpp>
#include <SGE/utils/text.hpp>

#include "ui.hpp"

inline constexpr size_t MAX_NODE_STACK_SIZE = 100;
inline constexpr size_t ARENA_CAPACITY = 1024 * 1024;

class Arena {
public:
    explicit Arena(size_t capacity) : m_capacity(capacity) {
        m_data = new uint8_t[capacity];
    }

    // Allocate raw memory from the arena
    void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) noexcept {
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
    std::function<void(sge::MouseButton)> on_click_callback = nullptr;

    std::vector<size_t> children{};
    
    UiRect padding;
    UiSize sizing;
    
    glm::vec2 pos = glm::vec2(0.0f);
    glm::vec2 size = glm::vec2(0.0f);
    
    void* custom_data = nullptr;
    size_t custom_data_size = 0;
    
    float gap = 0.0f;
    
    uint32_t type_id = 0;
    uint32_t z_index = 0;
    uint32_t text_data_index = 0;
    
    LayoutOrientation orientation = LayoutOrientation::Horizontal;
    std::optional<Alignment> self_alignment = Alignment::Start;
    Alignment horizontal_alignment = Alignment::Start;
    Alignment vertical_alignment = Alignment::Start;

    bool render = false;
    bool text_node = false;
};

static struct {
    size_t node_stack[MAX_NODE_STACK_SIZE];
    
    std::unordered_set<NodeID> hovered_ids{};
    std::unordered_set<NodeID> search_visited{};
    sge::SwapbackVector<Node*> search_stack{};
    sge::SwapbackVector<Node*> postorder_stack{};

    sge::SwapbackVector<Node*> growable_nodes{};
    
    Arena arena{ ARENA_CAPACITY };

    std::vector<Node> nodes{};
    std::vector<TextData> text_data{};
    std::vector<UiElement> render_elements{};
    
    size_t node_stack_size = 0;

    NodeID active_id{};

    bool any_hovered = false;
} state;

[[nodiscard]]
static inline bool NodeIsClickable(const Node& node) noexcept {
    return node.on_click_callback != nullptr;
}

static Node& TopNode() noexcept {
    SGE_ASSERT(state.node_stack_size > 0);

    const size_t index = state.node_stack[state.node_stack_size-1];
    return state.nodes[index];
}

static Node& ParentNode() noexcept {
    SGE_ASSERT(state.node_stack_size > 1);
    const size_t index = state.node_stack[state.node_stack_size-2];
    return state.nodes[index];
}

static void NodeAddElement(Node& parent, Node&& child) {
    SGE_ASSERT(!parent.text_node);

    const size_t index = state.nodes.size();
    parent.children.push_back(index);
    state.nodes.push_back(std::move(child));
}

static void PushNode(Node& parent, Node&& child) {
    SGE_ASSERT(!parent.text_node);

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

static inline std::string_view CopyStringToArena(std::string_view string) {
    char* str = static_cast<char*>(state.arena.allocate(string.size() + 1, alignof(std::string_view::value_type)));
    memcpy(str, string.data(), string.size());
    str[string.size()] = '\0'; // Add a null terminator just in case
    return { str, string.size() + 1 };
}

[[nodiscard]]
bool UI::IsMouseOverUi() noexcept {
    return state.any_hovered;
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
    state.search_visited.insert(state.nodes[0].unique_id);

    state.postorder_stack.clear();

    while (!state.search_stack.empty()) {
        Node* current_node = state.search_stack.back();
        state.search_stack.pop_back();

        if (current_node->text_node)
            continue;

        const sge::Rect element_rect = sge::Rect::from_top_left(current_node->pos, current_node->size);
        const bool hovered = element_rect.contains(sge::Input::MouseScreenPosition());

        const bool clickable = NodeIsClickable(*current_node);

        if (hovered) {
            state.hovered_ids.insert(current_node->unique_id);
            state.any_hovered = state.any_hovered || clickable;
        }

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
    state.arena.reset();
    state.nodes.clear();
    state.render_elements.clear();
    state.text_data.clear();

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
        node.z_index = 0;
        node.orientation = desc.orientation();
        node.horizontal_alignment = desc.vertical_alignment();
        node.render = false;
        node.text_node = false;
    }
    PushNode(std::move(node));
}

static ElementID GenerateId(Node& parent) {
    SGE_ASSERT(!parent.text_node);
    ElementID id = HashNumber(parent.children.size(), parent.unique_id.id);
    return id;
}

void UI::BeginElement(uint32_t type_id, const ElementDesc& desc, bool render) {
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

    Node new_node;
    {
        new_node.children = {};
        new_node.pos = glm::vec2(0.0f);
        new_node.size = size;
        new_node.padding = desc.padding;
        new_node.sizing = desc.size;
        new_node.custom_data = nullptr;
        new_node.custom_data_size = 0;
        new_node.gap = desc.gap;
        new_node.unique_id = id;
        new_node.type_id = type_id;
        new_node.z_index = parent.z_index + 1;
        new_node.orientation = desc.orientation;
        new_node.horizontal_alignment = desc.horizontal_alignment;
        new_node.vertical_alignment = desc.vertical_alignment;
        new_node.self_alignment = desc.self_alignment;
        new_node.render = render;
    }
    PushNode(parent, std::move(new_node));
}

static void GrowElementsHorizontally(Node& parent) {
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

    if (state.growable_nodes.size() > 0) {
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

            width_to_add = std::min(width_to_add, remaining_width / state.growable_nodes.size());

            for (Node* child : state.growable_nodes) {
                if (sge::approx_equals(child->size.x, smallest)) {
                    child->size.x += width_to_add;
                    remaining_width -= width_to_add;
                }
            }
        }
    }
}

static void GrowElementsVertically(Node& parent) {
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
    
    if (state.growable_nodes.size() > 0) {
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

            height_to_add = std::min(height_to_add, remaining_height / state.growable_nodes.size());

            for (Node* child : state.growable_nodes) {
                if (sge::approx_equals(child->size.y, smallest)) {
                    child->size.y += height_to_add;
                    remaining_height -= height_to_add;
                }
            }
        }
    }
}

void UI::EndElement() {
    SGE_ASSERT(state.node_stack_size > 0);

    Node& node = TopNode();
    --state.node_stack_size;

    if (node.text_node) return;

    if (node.orientation == LayoutOrientation::Stack) {
        uint32_t z_index = node.z_index;
        for (uint32_t child_index : node.children) {
            Node& child = state.nodes[child_index];
            child.z_index = ++z_index;
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
    }
}

void UI::Text(uint32_t type_id, const sge::Font& font, const sge::RichTextSection* sections, const size_t count, const TextElementDesc& desc) {
    glm::vec2 measured_size = glm::vec2(0.0f);

    for (size_t i = 0; i < count; ++i) {
        const sge::RichTextSection& section = sections[i];
        measured_size = glm::max(measured_size, calculate_text_bounds(font, section.text.size(), section.text.data(), section.size));
    }

    // Copy section contents
    sge::RichTextSection* arena_sections = static_cast<sge::RichTextSection*>(state.arena.allocate(sizeof(sge::RichTextSection) * count, alignof(sge::RichTextSection)));
    for (size_t i = 0; i < count; ++i) {
        const sge::RichTextSection& section = sections[i];
        arena_sections->size = section.size;
        arena_sections->color = section.color;
        arena_sections->text = CopyStringToArena(section.text);
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

    Node new_node;
    {
        new_node.children = {};
        new_node.pos = glm::vec2(0.0f);
        new_node.size = measured_size;
        new_node.sizing = UiSize::Fixed(measured_size);
        new_node.text_data_index = text_data_index;
        new_node.unique_id = id;
        new_node.type_id = type_id;
        new_node.z_index = parent.z_index + 1;
        new_node.render = true;
        new_node.text_node = true;
        new_node.self_alignment = desc.self_alignment;
    }
    NodeAddElement(parent, std::move(new_node));
}

static void FinalizeLayout() {
    state.search_stack.clear();
    state.search_stack.push_back(&state.nodes[0]);

    state.search_visited.clear();
    state.search_visited.insert(state.nodes[0].unique_id);

    while (!state.search_stack.empty()) {
        Node* current_node = state.search_stack.front();
        state.search_stack.pop_front();

        GrowElementsHorizontally(*current_node);
        GrowElementsVertically(*current_node);

        if (current_node->render) {
            TextData* text_data = nullptr;
            if (current_node->text_node) {
                text_data = &state.text_data[current_node->text_data_index];
            }

            state.render_elements.push_back(UiElement {
                .unique_id = current_node->unique_id,
                .position = current_node->pos,
                .size = current_node->size,
                .custom_data = current_node->custom_data,
                .custom_data_size = current_node->custom_data_size,
                .text_data = text_data,
                .type_id = current_node->type_id,
                .z_index = current_node->z_index
            });
        }

        if (current_node->text_node)
            continue;

        glm::vec2 pos = current_node->pos + glm::vec2(current_node->padding.left(), current_node->padding.top());

        float remaining_width = current_node->size.x - current_node->padding.left() - current_node->padding.right();
        float remaining_height = current_node->size.y - current_node->padding.top() - current_node->padding.bottom();

        for (uint32_t child_index : current_node->children) {
            Node& child = state.nodes[child_index];

            if (state.search_visited.contains(child.unique_id)) {
                continue;
            }

            state.search_stack.push_back(&child);
            state.search_visited.insert(child.unique_id);
            
            if (current_node->orientation == LayoutOrientation::Horizontal) {
                remaining_height = current_node->size.y - child.size.y - current_node->padding.top() - current_node->padding.bottom();
            } else if (current_node->orientation == LayoutOrientation::Vertical) {
                remaining_width = current_node->size.x - child.size.x - current_node->padding.left() - current_node->padding.right();
            } else if (current_node->orientation == LayoutOrientation::Stack) {
                remaining_width = current_node->size.x - child.size.x - current_node->padding.left() - current_node->padding.right();
                remaining_height = current_node->size.y - child.size.y - current_node->padding.top() - current_node->padding.bottom();
            }
            
            child.pos = pos;

            bool horizontally_aligned = false;
            bool vertically_aligned = false;

            if (child.self_alignment && current_node->orientation == LayoutOrientation::Horizontal) {
                if (child.self_alignment == Alignment::Center) {
                    child.pos.y += remaining_height * 0.5f;
                } else if (child.self_alignment == Alignment::End) {
                    child.pos.y += remaining_height;
                }
                vertically_aligned = true;
            }

            if (child.self_alignment && current_node->orientation == LayoutOrientation::Vertical) {
                if (child.self_alignment == Alignment::Center) {
                    child.pos.x += remaining_width * 0.5f;
                } else if (child.self_alignment == Alignment::End) {
                    child.pos.x += remaining_width;
                }
                horizontally_aligned = true;
            }

            if (!horizontally_aligned) {
                if (current_node->horizontal_alignment == Alignment::Center) {
                    child.pos.x += remaining_width * 0.5f;
                } else if (current_node->horizontal_alignment == Alignment::End) {
                    child.pos.x += remaining_width;
                }
            }

            if (!vertically_aligned) {
                if (current_node->vertical_alignment == Alignment::Center) {
                    child.pos.y += remaining_height * 0.5f;
                } else if (current_node->vertical_alignment == Alignment::End) {
                    child.pos.y += remaining_height;
                }
            }

            if (current_node->orientation == LayoutOrientation::Horizontal) {
                pos.x += child.size.x + current_node->gap;
                remaining_width -= child.size.x;
            } else if (current_node->orientation == LayoutOrientation::Vertical) {
                pos.y += child.size.y + current_node->gap;
                remaining_height -= child.size.y;
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

const std::vector<UiElement>& UI::Finish() {
    FinalizeLayout();
    return state.render_elements;
};

[[nodiscard]]
ElementID ID::Local(const std::string_view key) noexcept {
    const uint32_t parent_id = TopNode().unique_id.id;
    return HashString(CopyStringToArena(key), parent_id);
}

ElementID ID::Global(const std::string_view key, uint32_t index) noexcept {
    CopyStringToArena(key);
    return HashString(CopyStringToArena(key), index);
}