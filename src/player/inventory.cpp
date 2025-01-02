#include "inventory.hpp"
#include "../assert.hpp"

const Item* Inventory::get_item(uint8_t index) const {
    ASSERT(index < 50, "Index must be less than 50.")

    if (!m_items[index].has_value()) return nullptr;

    return &m_items[index].value();
}

Item* Inventory::get_item(uint8_t index) {
    ASSERT(index < 50, "Index must be less than 50.")

    if (!m_items[index].has_value()) return nullptr;

    return &m_items[index].value();
}

void Inventory::set_item(uint8_t index, const Item& item) {
    ASSERT(index < 50, "Index must be less than 50.")
    m_items[index] = item;
}