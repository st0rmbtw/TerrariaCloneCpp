#ifndef TERRARIA_ITEM_HPP
#define TERRARIA_ITEM_HPP

#pragma once

#include <stdint.h>
#include <string>

#include "../optional.hpp"

#include "block.hpp"

using ItemStack = uint16_t;

struct Item {
    uint16_t id;
    ItemStack max_stack;
    ItemStack stack;
    uint8_t swing_speed;
    uint8_t use_cooldown;
    uint8_t power;
    bool consumable;
    bool is_axe;
    bool is_pickaxe;
    bool is_hammer;
    tl::optional<BlockType> places_block;
    std::string name;

    inline Item with_stack(ItemStack stack) {
        Item new_item = *this;
        new_item.stack = std::min(max_stack, stack);
        return new_item;
    }

    inline Item with_max_stack() {
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

#endif