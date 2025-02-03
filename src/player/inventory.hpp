#pragma once

#ifndef INVENTORY_HPP
#define INVENTORY_HPP

#include <optional>

#include "../types/item.hpp"
#include "../engine/assert.hpp"

constexpr uint8_t CELLS_IN_ROW = 10;
constexpr uint8_t INVENTORY_ROWS = 5;
constexpr uint8_t TAKEN_ITEM_INDEX = 50;

struct ItemSlot {
    explicit ItemSlot(const std::optional<Item>& item, uint8_t index = 0) :
        item(item),
        index(index) {}

    [[nodiscard]] inline bool has_item() const { return item.has_value(); }

    const std::optional<Item>& item;
    uint8_t index = 0;
};

struct TakenItem {
    ItemSlot item_slot;
    uint8_t taken_from;
};

class Inventory {
public:
    Inventory() = default;

    inline void set_selected_slot(uint8_t index) {
        ASSERT(index < CELLS_IN_ROW, "Index must be less than 10.");
        m_selected_slot = index;
    }

    [[nodiscard]]
    inline ItemSlot get_item(uint8_t index) const {
        ASSERT(index < 51, "Index must be less than 50.");

        return ItemSlot(m_items[index], index);
    }

    [[nodiscard]]
    inline void set_item(uint8_t index, const Item& item) {
        ASSERT(index < 51, "Index must be less than 50.");
        m_items[index] = item;
    }

    [[nodiscard]]
    inline bool item_exits(uint8_t index) const {
        ASSERT(index < 51, "Index must be less than 50.");
        return m_items[index].has_value();
    }

    inline void take_or_put_item(uint8_t index) {
        ASSERT(index < 51, "Index must be less than 50.");
        m_taken_item_index = index;
        std::swap(m_items[index], m_items[TAKEN_ITEM_INDEX]);
    }

    inline void return_taken_item() {
        if (!m_items[TAKEN_ITEM_INDEX].has_value()) return;
        m_items[m_taken_item_index] = m_items[TAKEN_ITEM_INDEX];
        m_items[TAKEN_ITEM_INDEX] = std::nullopt;
    }

    inline void consume_item(uint8_t index) {
        ASSERT(index < 51, "Index must be less than 50.");
        std::optional<Item>& item = m_items[index];
        if (!item) return;

        if (!item->consumable) return;

        if (item->stack > 1) item->stack -= 1;
        else m_items[index] = std::nullopt;
    }

    [[nodiscard]] inline ItemSlot get_selected_item() const { return get_item(selected_slot()); }
    [[nodiscard]] inline uint8_t selected_slot() const { return m_selected_slot; }

    [[nodiscard]]
    inline ItemSlot taken_item() const {
        return ItemSlot(m_items[TAKEN_ITEM_INDEX], TAKEN_ITEM_INDEX);
    }

private:
    uint8_t m_selected_slot = 0;
    uint8_t m_taken_item_index = 0;
    std::optional<Item> m_items[51];
};

#endif