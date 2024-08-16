#include "inventory.hpp"
#include "../assert.hpp"

tl::optional<const Item&> Inventory::get_item(uint8_t index) const {
    ASSERT(index < 50, "Index must be less than 50.")

    if (m_items[index].is_none()) return tl::nullopt;

    return m_items[index].get();
}

tl::optional<Item&> Inventory::get_item(uint8_t index) {
    ASSERT(index < 50, "Index must be less than 50.")

    if (m_items[index].is_none()) return tl::nullopt;

    return m_items[index].get();
}

void Inventory::set_item(uint8_t index, const Item& item) {
    ASSERT(index < 50, "Index must be less than 50.")
    m_items[index] = item;
}