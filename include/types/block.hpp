#ifndef TERRARIA_BLOCK_HPP
#define TERRARIA_BLOCK_HPP

#pragma once

#include <stdint.h>
#include "optional.hpp"
#include "types/texture_atlas_pos.hpp"

enum class BlockType : uint8_t {
    Dirt = 0,
    Stone = 1,
    Grass = 2,
    Wood = 30
};

inline constexpr static bool block_is_stone(BlockType block_type) {
    switch (block_type) {
    case BlockType::Dirt:  return false;
    case BlockType::Grass: return false;
    case BlockType::Stone: return true;
    case BlockType::Wood:  return false;
    }
}

inline constexpr static int16_t block_hp(BlockType block_type) {
    switch (block_type) {
    case BlockType::Dirt:  case BlockType::Grass: return 50;
    case BlockType::Stone: case BlockType::Wood:  return 100;
    }
}

inline constexpr static tl::optional<BlockType> block_merge_with(BlockType block_type) {
    switch (block_type) {
    case BlockType::Dirt:  return tl::nullopt;
    case BlockType::Grass: return BlockType::Dirt;
    case BlockType::Stone: return BlockType::Dirt;
    case BlockType::Wood:  return BlockType::Dirt;
    }
}

inline constexpr static bool block_merges_with(BlockType block, BlockType other) {
    const tl::optional<BlockType> this_merge = block_merge_with(block);
    const tl::optional<BlockType> other_merge = block_merge_with(other);

    if (other_merge == block) return true;
    if (this_merge == other) return true;
    if (this_merge.is_some() && other_merge.is_some() && this_merge == other_merge) return true;

    return false;
}

inline constexpr static const char* block_type_name(BlockType block_type) {
    switch (block_type) {
    case BlockType::Dirt:  return "Dirt";
    case BlockType::Grass: return "Grass";
    case BlockType::Stone: return "Stone";
    case BlockType::Wood:  return "Wood";
    }
}

inline constexpr static bool block_dusty(BlockType block_type) {
    switch (block_type) {
    case BlockType::Dirt:  return true;
    case BlockType::Grass: return false;
    case BlockType::Stone: return false;
    case BlockType::Wood:  return false;
    }
}

struct Block {
    BlockType type;
    int16_t hp;
    uint8_t variant;
    TextureAtlasPos atlas_pos;
    uint8_t merge_id = 0xFF;
    bool is_merged = false;

    Block(BlockType block_type) : 
        type(block_type),
        hp(block_hp(block_type)),
        variant(static_cast<uint8_t>(rand() % 3)),
        atlas_pos{} {}
};

template <class T>
struct Neighbors {
    tl::optional<T> top;
    tl::optional<T> bottom;
    tl::optional<T> left;
    tl::optional<T> right;
    tl::optional<T> top_left;
    tl::optional<T> top_right;
    tl::optional<T> bottom_left;
    tl::optional<T> bottom_right;

    [[nodiscard]]
    inline bool any_not_exists() const {
        return top.is_none() || bottom.is_none() || left.is_none() || 
            right.is_none() || top_left.is_none() || top_right.is_none() || 
            bottom_left.is_none() || bottom_right.is_none();
    }

    [[nodiscard]]
    inline bool any_exists() const {
        return top.is_some() || bottom.is_some() || left.is_some() || 
            right.is_some() || top_left.is_some() || top_right.is_some() || 
            bottom_left.is_some() || bottom_right.is_some();
    }
};

#endif