#pragma once

#ifndef INVENTORY_HPP_
#define INVENTORY_HPP_

#include <optional>

#include <SGE/assert.hpp>

#include "../types/item.hpp"

using ItemIndex = uint8_t;

static constexpr ItemIndex ITEM_COUNT = 51;
static constexpr ItemIndex CELLS_IN_ROW = 10;
static constexpr ItemIndex INVENTORY_ROWS = 5;
static constexpr ItemIndex TAKEN_ITEM_INDEX = 50;

struct ItemSlot {
    explicit ItemSlot(const std::optional<Item>& item, ItemIndex index = 0) noexcept :
        item(item),
        index(index) {}

    [[nodiscard]]
    inline bool has_item() const noexcept {
        return item.has_value();
    }

    const std::optional<Item>& item;
    ItemIndex index = 0;
};

struct TakenItem {
    ItemSlot item_slot;
    ItemIndex taken_from;
};

class Inventory {
public:
    Inventory() = default;

    inline void set_selected_slot(ItemIndex index) noexcept {
        SGE_ASSERT(index < CELLS_IN_ROW);
        m_selected_slot = index;
    }

    [[nodiscard]]
    inline ItemSlot get_item(ItemIndex index) const noexcept {
        SGE_ASSERT(index < ITEM_COUNT);
        return ItemSlot(m_items[index], index);
    }

    inline std::optional<Item> remove_item(ItemIndex index) noexcept {
        SGE_ASSERT(index < ITEM_COUNT);
        const std::optional<Item> item = m_items[index];
        m_items[index] = std::nullopt;
        return item;
    }

    inline void set_item(ItemIndex index, const Item& item) noexcept {
        SGE_ASSERT(index < ITEM_COUNT);
        m_items[index] = item;
    }

    [[nodiscard]]
    inline bool item_exits(ItemIndex index) const noexcept {
        SGE_ASSERT(index < ITEM_COUNT);
        return m_items[index].has_value();
    }

    inline void put_item(ItemIndex index) {
        SGE_ASSERT(index < ITEM_COUNT);
        move_item_stack(TAKEN_ITEM_INDEX, index);
    }

    inline void take_item(ItemIndex index, ItemStack count = 0) {
        SGE_ASSERT(index < ITEM_COUNT);
        move_item_stack(index, TAKEN_ITEM_INDEX, count);
    }

    inline std::optional<Item> return_taken_item() noexcept {
        if (!m_items[TAKEN_ITEM_INDEX].has_value()) return std::nullopt;

        if (!m_items[m_taken_item_index]->has_space()) {
            const ItemStack remaining = add_item_stack(m_items[TAKEN_ITEM_INDEX].value());
            if (remaining > 0) {
                return m_items[m_taken_item_index]->with_stack(remaining);
            }
        }

        m_items[TAKEN_ITEM_INDEX] = std::nullopt;

        return std::nullopt;
    }

    inline void consume_item(ItemIndex index) noexcept {
        SGE_ASSERT(index < ITEM_COUNT);

        std::optional<Item>& item = m_items[index];
        if (!item) return;

        if (!item->consumable) return;

        update_item_stack(index, item->stack - 1);
    }

    inline ItemStack move_item_stack(ItemIndex from_index, ItemIndex to_index, ItemStack count = 0) {
        SGE_ASSERT(from_index < ITEM_COUNT);
        SGE_ASSERT(to_index < ITEM_COUNT);

        std::optional<Item>& from = m_items[from_index];
        if (!from.has_value())
            return count;

        std::optional<Item>& to = m_items[to_index];

        if (count == 0)
            count = from->stack;

        SGE_ASSERT(count <= from->stack);

        ItemStack remaining = count;

        if (to.has_value()) {
            if (from->id != to->id)
                return count;

            const ItemStack took = std::min(count, static_cast<ItemStack>(to->max_stack - to->stack));
            to->stack += took;
            remaining -= took;
        } else {
            m_items[to_index] = from->with_stack(count);
            remaining -= count;
        }

        update_item_stack(from_index, from->stack - count - remaining);

        return remaining;
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
                remaining = new_stack - item->max_stack;
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
    inline ItemIndex selected_slot() const noexcept {
        return m_selected_slot;
    }

    [[nodiscard]]
    inline ItemSlot taken_item() const noexcept {
        return ItemSlot(m_items[TAKEN_ITEM_INDEX], TAKEN_ITEM_INDEX);
    }


    [[nodiscard]]
    bool has_taken_item() const noexcept {
        return m_items[TAKEN_ITEM_INDEX].has_value();
    }

private:

    void update_item_stack(ItemIndex index, ItemStack stack) {
        if (stack > 0)
            m_items[index]->stack = stack;
        else
            m_items[index] = std::nullopt;
    }

private:
    ItemIndex m_selected_slot = 0;
    ItemIndex m_taken_item_index = 0;
    std::optional<Item> m_items[ITEM_COUNT];
};

#endif