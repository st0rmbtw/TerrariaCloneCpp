#ifndef TYPES_ITEM_HPP
#define TYPES_ITEM_HPP

#pragma once

#include <cstdint>
#include <string_view>
#include <optional>

#include "block.hpp"
#include "wall.hpp"
#include "tool_flags.hpp"

struct PlacesTile {
    enum Type : uint8_t {
        Block = 0,
        Wall
    };

    PlacesTile(BlockType tile_type) :
        tile(tile_type),
        type(Type::Block) {}

    PlacesTile(WallType wall_type) :
        wall(wall_type),
        type(Type::Wall) {}

    union {
        BlockType tile;
        WallType wall;
    };
    Type type;
};

using ItemStack = uint16_t;

class ItemId {
public:
    using IdType = uint16_t;

    enum : IdType {
        DirtBlock = 2,
        StoneBlock = 3,
        Torch = 8,
        WoodBlock = 9,
        WoodWall = 93,
        CopperHammer = 3505,
        CopperAxe = 3506,
        CopperPickaxe = 3509
    };

    ItemId(IdType id) : m_id(id) {}

    inline constexpr operator IdType() const { return m_id; }
private:
    IdType m_id;
};

enum class HoldStyle : uint8_t {
    None = 0,
    HoldFront,
};

struct Item {
    std::string_view name;
    ItemId id;
    ItemStack max_stack;
    ItemStack stack;
    std::optional<PlacesTile> places_tile;
    uint8_t swing_speed;
    uint8_t use_cooldown;
    uint8_t power;
    bool consumable;
    HoldStyle hold_style;
    uint8_t tool_flags;

    [[nodiscard]]
    inline constexpr bool is_axe() const {
        return (tool_flags & ToolFlags::Axe) == ToolFlags::Axe;
    }

    [[nodiscard]]
    inline constexpr bool is_pickaxe() const {
        return (tool_flags & ToolFlags::Pickaxe) == ToolFlags::Pickaxe;
    }

    [[nodiscard]]
    inline constexpr bool is_hammer() const {
        return (tool_flags & ToolFlags::Hammer) == ToolFlags::Hammer;
    }

    [[nodiscard]]
    inline constexpr bool has_space() const {
        return stack < max_stack;
    }

    [[nodiscard]]
    inline Item with_stack(ItemStack stack) const {
        Item new_item = *this;
        new_item.stack = std::min(max_stack, stack);
        return new_item;
    }

    [[nodiscard]]
    inline Item with_max_stack() const {
        Item new_item = *this;
        new_item.stack = max_stack;
        return new_item;
    }
};

extern Item ITEM_COPPER_PICKAXE;
extern Item ITEM_COPPER_AXE;
extern Item ITEM_COPPER_HAMMER;
extern Item ITEM_DIRT_BLOCK;
extern Item ITEM_STONE_BLOCK;
extern Item ITEM_WOOD_BLOCK;
extern Item ITEM_TORCH;
extern Item ITEM_WOOD_WALL;

#endif