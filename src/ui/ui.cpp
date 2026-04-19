#include <algorithm>
#include <cstddef>
#include <deque>
#include <limits>
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
#include <SGE/utils/bitflags.hpp>

#include "../utils/data/small_vector.hpp"
#include "arena.hpp"
#include "ui.hpp"

inline constexpr size_t MAX_NODE_STACK_SIZE = 100;
inline constexpr size_t ARENA_CAPACITY = 1024 * 1024;

inline constexpr float FLOAT_EPSILON = std::numeric_limits<float>::epsilon();

using NodeID = ElementID;

template <typename T>
struct ArenaAllocator {
public:
    using value_type = T;

    Arena* arena = nullptr;

    ArenaAllocator(Arena& arena) : arena{ &arena } {}
    
    template <typename U>
    ArenaAllocator(const ArenaAllocator<U>& other) : arena(other.arena) {}

    value_type* allocate(std::size_t n) {
        return arena->allocate<value_type>(n);
    }

    void deallocate(value_type*, std::size_t) {
        // do nothing
    }

    ~ArenaAllocator() {
        arena = nullptr;
    }
    
    bool operator==(const ArenaAllocator&) const noexcept { return true; }
    bool operator!=(const ArenaAllocator&) const noexcept { return false; }
};

enum class NodeFlags : uint8_t {
    Render = 0,
    TextNode,
    Hoverable,
    Scrollable,
};

struct Node {
    NodeID unique_id{};
    std::function<void(sge::MouseButton)> on_click_callback = nullptr;

    // Not using std::vector here because it doesn't play nice with ArenaAllocator
    gch::small_vector<uint32_t, 16, ArenaAllocator<uint32_t>> children;
    
    UiRect padding;
    UiSize sizing;
    
    glm::vec2 pos = glm::vec2(0.0f);
    glm::vec2 size = glm::vec2(0.0f);
    glm::vec2 min_size = glm::vec2(0.0f);
    glm::vec2 max_size = glm::vec2(FLT_MAX);
    glm::vec2 offset = glm::vec2(0.0f);
    
    void* custom_data = nullptr;
    size_t custom_data_size = 0;
    
    float gap = 0.0f;
    float scroll_max = 0.0f;
    
    uint32_t type_id = -1;
    uint32_t z_index = 0;
    uint32_t text_data_index = 0;
    
    std::optional<Alignment> self_alignment = Alignment::Start;
    LayoutOrientation orientation = LayoutOrientation::Horizontal;
    Alignment horizontal_alignment = Alignment::Start;
    Alignment vertical_alignment = Alignment::Start;

    sge::BitFlags<NodeFlags> flags;

    Node(Arena& arena) : children{ arena } {}

    [[nodiscard]]
    inline bool render() const noexcept {
        return flags[NodeFlags::Render];
    }

    [[nodiscard]]
    inline bool is_text_node() const noexcept {
        return flags[NodeFlags::TextNode];
    }

    [[nodiscard]]
    inline bool hoverable() const noexcept {
        return flags[NodeFlags::Hoverable];
    }

    [[nodiscard]]
    inline bool scrollable() const noexcept {
        return flags[NodeFlags::Scrollable];
    }

    [[nodiscard]]
    inline bool clickable() const noexcept {
        return on_click_callback != nullptr;
    }
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
    uint32_t id_stack[MAX_NODE_STACK_SIZE];
    
    std::unordered_map<uint32_t, Node> previous_nodes{};
    std::unordered_map<uint32_t, Node> nodes{};
    std::unordered_set<NodeID> hovered_ids{};
    std::unordered_set<NodeID> search_visited{};
    std::unordered_map<NodeID, ScrollData> scroll_data{};

    std::deque<Node*> search_stack{};
    std::deque<SearchStackFrame> search_stack_frame{};

    NodeID clicked_clickable_id{};
    NodeID clicked_focusable_id{};
    NodeID focused_id{};

    std::vector<Node*> clickable_stack{};
    std::vector<Node*> focusable_stack{};

    sge::SwapbackVector<Node*> growable_nodes{};
    
    std::vector<TextData> text_data{};
    std::vector<UiElement> render_elements{};

    Arena arena{ ARENA_CAPACITY };
    
    size_t node_stack_size = 0;

    bool any_hovered = false;
} state;

static inline Node& GetNode(uint32_t id) noexcept {
    return state.nodes.find(id)->second;
}

static Node& TopNode() noexcept {
    SGE_ASSERT(state.node_stack_size > 0);
    const uint32_t id = state.id_stack[state.node_stack_size-1];
    return GetNode(id);
}

static Node& ParentNode() noexcept {
    SGE_ASSERT(state.node_stack_size > 1);
    const uint32_t id = state.id_stack[state.node_stack_size-2];
    return GetNode(id);
}

static void NodeAddElement(Node& parent, Node&& child) {
    SGE_ASSERT(!parent.is_text_node());
    SGE_ASSERT(state.nodes.size() < UINT32_MAX);
    SGE_ASSERT(state.node_stack_size < MAX_NODE_STACK_SIZE);

    const uint32_t id = child.unique_id.id;
    parent.children.push_back(id);
    state.nodes.emplace(id, std::move(child));
}

static void PushNode(Node& parent, Node&& child) {
    SGE_ASSERT(!parent.is_text_node());
    SGE_ASSERT(state.nodes.size() < UINT32_MAX);
    SGE_ASSERT(state.node_stack_size < MAX_NODE_STACK_SIZE);

    const uint32_t id = child.unique_id.id;
    parent.children.push_back(id);
    
    state.id_stack[state.node_stack_size] = id;
    state.node_stack_size += 1;

    state.nodes.emplace(id, std::move(child));
}

static void PushNode(Node&& node) {
    SGE_ASSERT(state.nodes.size() < UINT32_MAX);
    SGE_ASSERT(state.node_stack_size < MAX_NODE_STACK_SIZE);

    const uint32_t id = node.unique_id.id;
    state.nodes.emplace(id, std::move(node));

    state.id_stack[state.node_stack_size] = id;
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
    return { str, string.size() };
}

static bool CheckIfPressed(const NodeID id, const sge::Rect element_rect, const sge::MouseButton button) noexcept {
    const bool hovered = element_rect.contains(sge::Input::CursorPosition());

    if (state.clicked_clickable_id.id == 0) {
        if (hovered && sge::Input::JustPressed(button)) {
            state.clicked_clickable_id = id;
        }
    } else if (state.clicked_clickable_id == id) {
        if (sge::Input::JustReleased(button)) {
            state.clicked_clickable_id = NodeID();
            if (hovered) {
                return true;
            }
        }
    }

    return false;
}

static bool CheckIfFocused(const NodeID id, const sge::Rect element_rect) {
    const bool hovered = element_rect.contains(sge::Input::CursorPosition());

    if (state.clicked_focusable_id.id == 0) {
        if (hovered && sge::Input::JustPressed(sge::MouseButton::Left)) {
            state.clicked_focusable_id = id;
        }
    } else if (state.clicked_focusable_id == id) {
        if (sge::Input::JustReleased(sge::MouseButton::Left)) {
            state.clicked_focusable_id = NodeID();
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
    state.search_stack.push_back(&GetNode(0));

    state.search_visited.clear();

    state.clickable_stack.clear();
    state.focusable_stack.clear();

    while (!state.search_stack.empty()) {
        Node* current_node = state.search_stack.back();
        state.search_stack.pop_back();

        if (state.search_visited.contains(current_node->unique_id)) {
            continue;
        }

        state.search_visited.insert(current_node->unique_id);

        if (current_node->is_text_node())
            continue;

        const sge::Rect element_rect = sge::Rect::from_top_left(current_node->pos, current_node->size);
        const bool hovered = element_rect.contains(sge::Input::CursorPosition());

        const bool clickable = current_node->clickable();

        if (hovered) {
            state.hovered_ids.insert(current_node->unique_id);
            state.any_hovered = state.any_hovered || clickable || current_node->hoverable();

            if (current_node->render()) {
                ScrollData& scroll_data = state.scroll_data[current_node->unique_id];
                for (float delta : sge::Input::ScrollEvents()) {
                    scroll_data.offset += delta * 1000.0f * sge::Time::DeltaSeconds();
                    scroll_data.offset = std::clamp(scroll_data.offset, -current_node->scroll_max, 0.0f);
                }
            }
        }

        if (clickable) {
            state.clickable_stack.push_back(current_node);
        }

        state.focusable_stack.push_back(current_node);

        for (uint32_t child_index : current_node->children) {
            Node& child = GetNode(child_index);
            state.search_stack.push_back(&child);
        }
    }

    while (!state.focusable_stack.empty()) {
        Node& node = *state.focusable_stack.back();
        state.focusable_stack.pop_back();

        const sge::Rect element_rect = sge::Rect::from_top_left(node.pos, node.size);

        if (CheckIfFocused(node.unique_id, element_rect)) {
            state.focused_id = node.unique_id;
            break;
        }
    }

    while (!state.clickable_stack.empty()) {
        Node& node = *state.clickable_stack.back();
        state.clickable_stack.pop_back();

        const sge::Rect element_rect = sge::Rect::from_top_left(node.pos, node.size);

        if (node.clickable() && CheckIfPressed(node.unique_id, element_rect, sge::MouseButton::Left)) {
            node.on_click_callback(sge::MouseButton::Left);
            break;
        }

        if (node.clickable() && CheckIfPressed(node.unique_id, element_rect, sge::MouseButton::Right)) {
            node.on_click_callback(sge::MouseButton::Right);
            break;
        }

        if (node.clickable() && CheckIfPressed(node.unique_id, element_rect, sge::MouseButton::Middle)) {
            node.on_click_callback(sge::MouseButton::Middle);
            break;
        }
    }
}

void UI::Start(const RootDesc& desc) {
    ZoneScoped;

    state.arena.clear();
    state.nodes.swap(state.previous_nodes);
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
    SGE_ASSERT(!parent.is_text_node());
    ElementID id = HashNumber(parent.children.size(), parent.unique_id.id);
    return id;
}

void UI::BeginElement(uint32_t type_id, const ElementDesc& desc, bool render) {
    ZoneScoped;

    glm::vec2 size = glm::vec2(0.0f);
    glm::vec2 min_size = glm::vec2(desc.min_width, desc.min_height);
    glm::vec2 max_size = glm::vec2(desc.max_width, desc.max_height);

    if (desc.size.width().type() == Sizing::Type::Fixed) {
        size.x = desc.size.width().value();
        min_size.x = size.x;
        max_size.x = size.x;
    }
    if (desc.size.height().type() == Sizing::Type::Fixed) {
        size.y = desc.size.height().value();
        min_size.y = size.y;
        max_size.y = size.y;
    }

    Node& parent = TopNode();

    max_size = glm::min(max_size, parent.max_size - glm::vec2(parent.padding.width(), parent.padding.height()));

    NodeID id = desc.id;
    if (id.id == 0) {
        id = GenerateId(parent);
    }

    Node new_node(state.arena);
    {
        new_node.unique_id = id;
        new_node.size = size;
        new_node.min_size = min_size;
        new_node.max_size = max_size;
        new_node.offset = desc.offset;
        new_node.sizing = desc.size;
        new_node.padding = desc.padding;
        new_node.orientation = desc.orientation;
        new_node.horizontal_alignment = desc.horizontal_alignment;
        new_node.vertical_alignment = desc.vertical_alignment;
        new_node.self_alignment = desc.self_alignment;
        new_node.gap = desc.gap;
        new_node.flags.set(NodeFlags::Hoverable, desc.hoverable);
        new_node.flags.set(NodeFlags::Scrollable, desc.scrollable);
        new_node.flags.set(NodeFlags::Render, render);
        new_node.type_id = type_id;
        new_node.z_index = parent.z_index + 1;
    }

    if (desc.scrollable && !state.scroll_data.contains(id)) {
        state.scroll_data[id] = ScrollData{};
    }

    PushNode(parent, std::move(new_node));
}

static void GrowElementsHorizontally(Node& parent) {
    ZoneScoped;

    if (parent.is_text_node()) return;
    if (parent.children.empty()) return;

    state.growable_nodes.clear();

    for (uint32_t child_index : parent.children) {
        Node& child = GetNode(child_index);
        if (child.sizing.width().type() == Sizing::Type::Fill) {
            state.growable_nodes.push_back(&child);
        }
    }

    if (state.growable_nodes.empty())
        return;

    if (parent.orientation == LayoutOrientation::Horizontal) {
        float remaining_width = parent.size.x - parent.padding.width();
        remaining_width -= (parent.children.size() - 1) * parent.gap;
        for (uint32_t child_index : parent.children) {
            Node& child = GetNode(child_index);
            remaining_width -= child.size.x;
        }

        while (remaining_width > FLOAT_EPSILON) {
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

            for (size_t i = 0; i < state.growable_nodes.size(); i++) {
                Node* node = state.growable_nodes[i];

                if (sge::approx_equals(node->size.x, smallest)) {
                    node->size.x += width_to_add;
                    if (node->size.x > node->max_size.x) {
                        node->size.x = node->max_size.x;
                        state.growable_nodes.erase(i--);
                    }
                    remaining_width -= width_to_add;
                }
            }
        }
    } else {
        const float max_width = parent.size.x - parent.padding.width();

        for (Node* node : state.growable_nodes) {
            node->size.x = std::min(max_width, node->max_size.x);
        }
    }
}

static void GrowElementsVertically(Node& parent) {
    ZoneScoped;

    if (parent.is_text_node()) return;
    if (parent.children.empty()) return;

    state.growable_nodes.clear();

    for (uint32_t child_index : parent.children) {
        Node& child = GetNode(child_index);
        if (child.sizing.height().type() == Sizing::Type::Fill) {
            state.growable_nodes.push_back(&child);
        }
    }

    if (state.growable_nodes.empty())
        return;

    if (parent.orientation == LayoutOrientation::Vertical) {
        float remaining_height = parent.size.y - parent.padding.height();

        remaining_height -= (parent.children.size() - 1) * parent.gap;
        for (uint32_t child_index : parent.children) {
            Node& child = GetNode(child_index);
            remaining_height -= child.size.y;
        }

        while (remaining_height > FLOAT_EPSILON) {
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

            for (size_t i = 0; i < state.growable_nodes.size(); i++) {
                Node* node = state.growable_nodes[i];

                if (sge::approx_equals(node->size.y, smallest)) {
                    node->size.y += height_to_add;
                    if (node->size.y > node->max_size.y) {
                        node->size.y = node->max_size.y;
                        state.growable_nodes.erase(i--);
                    }
                    remaining_height -= height_to_add;
                }
            }
        }
    } else {
        const float max_height = parent.size.y - parent.padding.height();

        for (Node* node : state.growable_nodes) {
            node->size.y = std::min(max_height, node->max_size.y);
        }
    }
}

static inline Node& GetChild(Node& node, uint32_t index) {
    const uint32_t idx = node.children[index];
    return GetNode(idx);
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
            Node& child = GetNode(child_index);
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
            Node& child = GetNode(node.children[i + 1]);
            PropagateZIndex(child, ++z_index);
        } while (++i < node.children.size() - 1);
    } else {
        for (uint32_t child_index : node.children) {
            Node& child = GetNode(child_index);
            PropagateZIndex(child, z_index + 1);
        }
    }
}

void UI::EndElement() {
    ZoneScoped;

    SGE_ASSERT(state.node_stack_size > 0);

    Node& node = TopNode();
    --state.node_stack_size;

    if (node.is_text_node()) return;

    PropagateZIndex(node, node.z_index);

    const bool width_fixed = node.sizing.width().type() == Sizing::Type::Fixed;
    const bool height_fixed = node.sizing.height().type() == Sizing::Type::Fixed;

    const float horizontal_padding = node.padding.width();
    const float vertical_padding = node.padding.height();

    glm::vec2 min_size = glm::vec2(0.0f, 0.0f);

    if (!width_fixed)
        min_size.x += horizontal_padding;

    if (!height_fixed)
        min_size.y += horizontal_padding;

    if (node.orientation == LayoutOrientation::Horizontal) {
        if (!width_fixed) {
            const float gap = (std::max<size_t>(node.children.size(), 1) - 1) * node.gap;
            min_size.x += gap;
        }

        for (uint32_t child_index : node.children) {
            const Node& child = GetNode(child_index);

            if (!width_fixed)
                min_size.x += child.size.x;

            if (!height_fixed)
                min_size.y = std::max(min_size.y, child.size.y + vertical_padding);
        }
    } else if (node.orientation == LayoutOrientation::Vertical) {
        if (!height_fixed) {
            const float gap = (std::max<size_t>(node.children.size(), 1) - 1) * node.gap;
            min_size.y += gap;
        }

        for (uint32_t child_index : node.children) {
            const Node& child = GetNode(child_index);

            if (!width_fixed)
                min_size.x = std::max(min_size.x, child.size.x + horizontal_padding);

            if (!height_fixed)
                min_size.y += child.size.y;
        }
    } else if (node.orientation == LayoutOrientation::Stack) {
        for (uint32_t child_index : node.children) {
            const Node& child = GetNode(child_index);

            if (!width_fixed)
                min_size.x = std::max(min_size.x, child.size.x + horizontal_padding);

            if (!height_fixed)
                min_size.y = std::max(min_size.y, child.size.y + vertical_padding);
        }
    }

    if (!width_fixed) {
        node.size.x = std::max(node.min_size.x, min_size.x);
        node.min_size.x = std::max(node.min_size.x, min_size.x);
    }

    if (!height_fixed) {
        node.size.y = std::max(node.min_size.y, min_size.y);
        node.min_size.y = std::max(node.min_size.y, min_size.y);
    }

    node.size = glm::min(node.size, node.max_size);
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
        new_node.min_size = measured_size;
        new_node.offset = desc.offset;
        new_node.sizing = UiSize::Fixed(measured_size);
        new_node.text_data_index = text_data_index;
        new_node.type_id = type_id;
        new_node.z_index = parent.z_index + 1;
        new_node.flags.set(NodeFlags::Render, true);
        new_node.flags.set(NodeFlags::TextNode, true);
        new_node.self_alignment = desc.self_alignment;
    }
    NodeAddElement(parent, std::move(new_node));
}

static void FinalizeLayout() {
    ZoneScoped;

    state.search_stack.clear();
    state.search_stack.push_back(&GetNode(0));

    state.search_visited.clear();
    state.search_visited.insert(GetNode(0).unique_id);

    while (!state.search_stack.empty()) {
        Node* current_node = state.search_stack.front();
        state.search_stack.pop_front();

        if (current_node->is_text_node())
            continue;

        GrowElementsHorizontally(*current_node);
        GrowElementsVertically(*current_node);

        const LayoutOrientation parent_orientation = current_node->orientation;
        const glm::vec2 parent_size = current_node->size;
        const UiRect parent_padding = current_node->padding;
        const Alignment parent_horizontal_alignment = current_node->horizontal_alignment;
        const Alignment parent_vertical_alignment = current_node->vertical_alignment;
        const float parent_gap = current_node->gap;

        glm::vec2 pos = current_node->pos + current_node->offset + glm::vec2(parent_padding.left(), parent_padding.top());

        if (current_node->scrollable()) {
            ScrollData& scroll_data = state.scroll_data[current_node->unique_id];
            if (parent_orientation == LayoutOrientation::Vertical) {
                pos.y += scroll_data.offset;
            } else if (parent_orientation == LayoutOrientation::Horizontal) {
                pos.x += scroll_data.offset;
            }
        }

        float max_scroll = parent_gap * (current_node->children.size() - 1);

        float remaining_width = parent_size.x - parent_padding.width();
        float remaining_height = parent_size.y - parent_padding.height();

        for (uint32_t child_index : current_node->children) {
            Node& child = GetNode(child_index);
            
            if (state.search_visited.contains(child.unique_id)) {
                continue;
            }

            child.pos = pos;
            
            state.search_stack.push_back(&child);
            state.search_visited.insert(child.unique_id);
            
            if (parent_orientation == LayoutOrientation::Horizontal) {
                pos.x += child.size.x + parent_gap;
                remaining_height = parent_size.y - child.size.y - parent_padding.height();
            } else if (parent_orientation == LayoutOrientation::Vertical) {
                pos.y += child.size.y + parent_gap;
                remaining_width = parent_size.x - child.size.x - parent_padding.width();
            } else if (parent_orientation == LayoutOrientation::Stack) {
                remaining_width = parent_size.x - child.size.x - parent_padding.width();
                remaining_height = parent_size.y - child.size.y - parent_padding.height();
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
                max_scroll += child.size.x;
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
                max_scroll += child.size.y;
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

        if (current_node->scrollable()) {
            if (parent_orientation == LayoutOrientation::Horizontal) {
                current_node->scroll_max = max_scroll - parent_size.x + parent_padding.width();
            } else if (parent_orientation == LayoutOrientation::Vertical) {
                current_node->scroll_max = max_scroll - parent_size.y + parent_padding.height();
            }
            if (current_node->scroll_max < 0.0f) {
                current_node->scroll_max = 0.0f;
            }
        }
    }

    state.search_stack_frame.clear();
    state.search_stack_frame.push_back(SearchStackFrame{ &GetNode(0), 0, SearchState::Done });

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
            Node& child = GetNode(child_index);

            const sge::Rect child_rect = sge::Rect::from_top_left(child.pos, child.size);

            if (child.scrollable()) {
                ScissorData* scissor_data = nullptr;
                scissor_data = state.arena.allocate<ScissorData>();
                scissor_data->area = sge::IRect::from_top_left(child.pos, glm::round(child.size));

                state.render_elements.push_back(UiElement {
                    .scissor_data = scissor_data,
                    .scissor_start = true,
                    .scissor_end = false
                });
            }

            if (child.render() && parent_rect.intersects(child_rect)) {
                TextData* text_data = nullptr;
                if (child.is_text_node()) {
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
            Node& child = GetNode(child_index);

            if (child.scrollable()) {
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

    void* custom_data = node.custom_data;
    if (custom_data_ptr != nullptr && custom_data_size > 0 && custom_data_alignment > 0) {
        if (node.custom_data == nullptr || node.custom_data_size != custom_data_size) {
            custom_data = state.arena.allocate(custom_data_size, custom_data_alignment);
        }
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

bool UI::IsFocused() noexcept {
    const Node& node = TopNode();
    return state.focused_id == node.unique_id;
}

[[nodiscard]]
const glm::vec2 UI::GetContentSize() noexcept {
    SGE_ASSERT(state.node_stack_size > 0);
    const NodeID id = GetElementID();
    
    const auto it = state.previous_nodes.find(id.id);

    if (it == state.previous_nodes.end())
        return glm::vec2(0.0f);

    const Node& node = it->second;
    return glm::max(node.size - glm::vec2(node.padding.width(), node.padding.height()), glm::vec2(0.0f));
}

[[nodiscard]]
const glm::vec2 UI::GetMaxSize() noexcept {
    const Node& node = TopNode();
    return node.max_size;
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