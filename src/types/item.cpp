#include "item.hpp"
#include "block.hpp"

Item ITEM_COPPER_AXE = {
    .name = "Copper Axe",
    .id = ItemId::CopperAxe,
    .max_stack = 1,
    .stack = 1,
    .places_tile = std::nullopt,
    .swing_speed = 30,
    .use_cooldown = 21,
    .power = 35,
    .consumable = false,
    .hold_style = HoldStyle::None,
    .tool_flags = ToolFlags::Axe
};

Item ITEM_COPPER_PICKAXE = {
    .name = "Copper Pickaxe",
    .id = ItemId::CopperPickaxe,
    .max_stack = 1,
    .stack = 1,
    .places_tile = std::nullopt,
    .swing_speed = 23,
    .use_cooldown = 15,
    .power = 35,
    .consumable = false,
    .hold_style = HoldStyle::None,
    .tool_flags = ToolFlags::Pickaxe
};

Item ITEM_COPPER_HAMMER = {
    .name = "Copper Hammer",
    .id = ItemId::CopperHammer,
    .max_stack = 1,
    .stack = 1,
    .places_tile = std::nullopt,
    .swing_speed = 33,
    .use_cooldown = 23,
    .power = 35,
    .consumable = false,
    .hold_style = HoldStyle::None,
    .tool_flags = ToolFlags::Hammer
};

Item ITEM_DIRT_BLOCK = {
    .name = "Dirt",
    .id = ItemId::DirtBlock,
    .max_stack = 9999,
    .stack = 1,
    .places_tile = PlacesTile(TileType::Dirt),
    .swing_speed = 15,
    .use_cooldown = 0,
    .power = 0,
    .consumable = true,
    .hold_style = HoldStyle::None,
    .tool_flags = ToolFlags::None
};

Item ITEM_STONE_BLOCK = {
    .name = "Stone",
    .id = ItemId::StoneBlock,
    .max_stack = 9999,
    .stack = 1,
    .places_tile = PlacesTile(TileType::Stone),
    .swing_speed = 15,
    .use_cooldown = 0,
    .power = 0,
    .consumable = true,
    .hold_style = HoldStyle::None,
    .tool_flags = ToolFlags::None
};

Item ITEM_TORCH = {
    .name = "Torch",
    .id = ItemId::Torch,
    .max_stack = 9999,
    .stack = 1,
    .places_tile = PlacesTile(TileType::Torch),
    .swing_speed = 15,
    .use_cooldown = 0,
    .power = 0,
    .consumable = true,
    .hold_style = HoldStyle::HoldFront,
    .tool_flags = ToolFlags::None
};

Item ITEM_WOOD_BLOCK = {
    .name = "Wood",
    .id = ItemId::WoodBlock,
    .max_stack = 9999,
    .stack = 1,
    .places_tile = PlacesTile(TileType::Wood),
    .swing_speed = 15,
    .use_cooldown = 0,
    .power = 0,
    .consumable = true,
    .hold_style = HoldStyle::None,
    .tool_flags = ToolFlags::None
};

Item ITEM_WOOD_WALL = {
    .name = "Wood Wall",
    .id = ItemId::WoodWall,
    .max_stack = 9999,
    .stack = 1,
    .places_tile = PlacesTile(WallType::WoodWall),
    .swing_speed = 15,
    .use_cooldown = 0,
    .power = 0,
    .consumable = true,
    .hold_style = HoldStyle::None,
    .tool_flags = ToolFlags::None
};