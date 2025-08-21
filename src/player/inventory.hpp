#pragma once

#ifndef INVENTORY_HPP_
#define INVENTORY_HPP_

#include <optional>

#include <SGE/assert.hpp>

#include "../types/item.hpp"

static constexpr uint8_t ITEM_COUNT = 51;
static constexpr uint8_t CELLS_IN_ROW = 10;
static constexpr uint8_t INVENTORY_ROWS = 5;
static constexpr uint8_t TAKEN_ITEM_INDEX = 50;

struct ItemSlot {
    explicit ItemSlot(const std::optional<Item>& item, uint8_t index = 0) noexcept :
        item(item),
        index(index) {}

    [[nodiscard]]
    inline bool has_item() const noexcept {
        return item.has_value();
    }

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

    inline void set_selected_slot(uint8_t index) noexcept {
        SGE_ASSERT(index < CELLS_IN_ROW);
        m_selected_slot = index;
    }

    [[nodiscard]]
    inline ItemSlot get_item(uint8_t index) const noexcept {
        SGE_ASSERT(index < 51);
        return ItemSlot(m_items[index], index);
    }

    inline std::optional<Item> remove_item(uint8_t index) noexcept {
        SGE_ASSERT(index < 51);
        const std::optional<Item> item = m_items[index];
        m_items[index] = std::nullopt;
        return item;
    }

    inline void set_item(uint8_t index, const Item& item) noexcept {
        SGE_ASSERT(index < 51);
        m_items[index] = item;
    }

    [[nodiscard]]
    inline bool item_exits(uint8_t index) const noexcept {
        SGE_ASSERT(index < 51);
        return m_items[index].has_value();
    }

    inline void take_or_put_item(uint8_t index) {
        SGE_ASSERT(index < 51);
        m_taken_item_index = index;
        std::swap(m_items[index], m_items[TAKEN_ITEM_INDEX]);
    }

    inline void return_taken_item() noexcept {
        if (!m_items[TAKEN_ITEM_INDEX].has_value()) return;
        m_items[m_taken_item_index] = m_items[TAKEN_ITEM_INDEX];
        m_items[TAKEN_ITEM_INDEX] = std::nullopt;
    }

    inline void consume_item(uint8_t index) noexcept {
        SGE_ASSERT(index < 51);

        std::optional<Item>& item = m_items[index];
        if (!item) return;

        if (!item->consumable) return;

        if (item->stack > 1) item->stack -= 1;
        else m_items[index] = std::nullopt;
    }

    /// Returns the number of remaining items
    ItemStack add_item_stack(const Item& new_item) {
        ItemStack remaining = new_item.stack;

        for (std::optional<Item>& item : m_items) {
            if (!item.has_value()) {
                item = new_item.with_stack(remaining);
                remaining = 0;
                break;
            }

            const bool same_id = item->id == new_item.id;
            const bool not_full = item->has_space();

            if (!same_id || !not_full)
                continue;

            const ItemStack new_stack = item->stack + remaining;

            if (new_stack <= item->max_stack) {
                item->stack = new_stack;
                remaining = 0;
                break;
            } else {
                item->stack = item->max_stack;
                remaining -= new_stack - item->max_stack;
            }
        }

        return remaining;
    }

    [[nodiscard]]
    bool can_be_added(const Item& new_item) const {
        ItemStack remaining = new_item.stack;

        for (const std::optional<Item>& item : m_items) {
            if (!item.has_value())
                return true;

            if (remaining == 0)
                return true;

            if (new_item.id != item->id)
                continue;

            if (item->stack == item->max_stack)
                continue;

            const ItemStack new_stack = item->stack + remaining;

            if (new_stack <= item->max_stack)
                return true;
            
            remaining -= remaining % item->max_stack;
        }

        return false;
    }

    [[nodiscard]]
    inline ItemSlot get_selected_item() const noexcept {
        return get_item(selected_slot());
    }

    [[nodiscard]]
    inline uint8_t selected_slot() const noexcept {
        return m_selected_slot;
    }

    [[nodiscard]]
    inline ItemSlot taken_item() const noexcept {
        return ItemSlot(m_items[TAKEN_ITEM_INDEX], TAKEN_ITEM_INDEX);
    }

private:
    uint8_t m_selected_slot = 0;
    uint8_t m_taken_item_index = 0;
    std::optional<Item> m_items[ITEM_COUNT];
};

#endif