#include "item.hpp"

Item ITEM_COPPER_AXE = {
    .id = 3506,
    .max_stack = 1,
    .stack = 1,
    .swing_speed = 30,
    .use_cooldown = 21,
    .power = 35,
    .consumable = false,
    .is_axe = true,
    .is_pickaxe = false,
    .is_hammer = false,
    .places_block = std::nullopt,
    .name = "Copper Axe"
};

Item ITEM_COPPER_PICKAXE = {
    .id = 3509,
    .max_stack = 1,
    .stack = 1,
    .swing_speed = 23,
    .use_cooldown = 15,
    .power = 35,
    .consumable = false,
    .is_axe = false,
    .is_pickaxe = true,
    .is_hammer = false,
    .places_block = std::nullopt,
    .name = "Copper Pickaxe"
};

Item ITEM_COPPER_HAMMER = {
    .id = 3505,
    .max_stack = 1,
    .stack = 1,
    .swing_speed = 33,
    .use_cooldown = 23,
    .power = 35,
    .consumable = false,
    .is_axe = false,
    .is_pickaxe = false,
    .is_hammer = true,
    .places_block = std::nullopt,
    .name = "Copper Hammer"
};

Item ITEM_DIRT_BLOCK = {
    .id = 2,
    .max_stack = 9999,
    .stack = 1,
    .swing_speed = 15,
    .use_cooldown = 0,
    .power = 0,
    .consumable = true,
    .is_axe = false,
    .is_pickaxe = false,
    .is_hammer = false,
    .places_block = BlockType::Dirt,
    .name = "Dirt"
};

Item ITEM_STONE_BLOCK = {
    .id = 3,
    .max_stack = 9999,
    .stack = 1,
    .swing_speed = 15,
    .use_cooldown = 0,
    .power = 0,
    .consumable = true,
    .is_axe = false,
    .is_pickaxe = false,
    .is_hammer = false,
    .places_block = BlockType::Stone,
    .name = "Stone"
};

Item ITEM_WOOD_BLOCK = {
    .id = 9,
    .max_stack = 9999,
    .stack = 1,
    .swing_speed = 15,
    .use_cooldown = 0,
    .power = 0,
    .consumable = true,
    .is_axe = false,
    .is_pickaxe = false,
    .is_hammer = false,
    .places_block = BlockType::Wood,
    .name = "Wood"
};