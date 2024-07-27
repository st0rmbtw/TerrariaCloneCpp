#pragma once

#ifndef INVENTORY_HPP
#define INVENTORY_HPP

#include "types/item.hpp"
#include "optional.hpp"
#include "common.h"

constexpr uint8_t CELLS_IN_ROW = 10;
constexpr uint8_t INVENTORY_ROWS = 5;

class Inventory {
public:
    Inventory() :
        m_selected_slot(0),
        m_items() {}

    tl::optional<const Item&> get_item(uint8_t index) const;
    tl::optional<Item&> get_item(uint8_t index);

    void set_item(uint8_t index, const Item& item);

    inline uint8_t selected_slot(void) const { return m_selected_slot; }

    inline tl::optional<const Item&> get_selected_item(void) const {
        return get_item(selected_slot());
    }

    inline void set_selected_slot(uint8_t index) {
        ASSERT(index < CELLS_IN_ROW, "Index must be less than 10.")
        m_selected_slot = index;
    }

private:
    uint8_t m_selected_slot;
    tl::optional<Item> m_items[50];
};

#endif