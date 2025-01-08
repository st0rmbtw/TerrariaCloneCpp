#ifndef TYPES_BLOCK_HPP
#define TYPES_BLOCK_HPP

#pragma once

#include <stdint.h>
#include <optional>

#include "texture_atlas_pos.hpp"
#include "../assets.hpp"

enum class BlockType : uint8_t {
    Dirt = 0,
    Stone = 1,
    Grass = 2,
    Wood = 30
};

inline constexpr static bool block_is_stone(BlockType block_type) {
    switch (block_type) {
    case BlockType::Stone: return true;
    default: return false;
    }
}

inline constexpr static int16_t block_hp(BlockType block_type) {
    switch (block_type) {
    case BlockType::Dirt:  case BlockType::Grass: return 50;
    case BlockType::Stone: case BlockType::Wood:  return 100;
    }
}

inline constexpr static std::optional<BlockType> block_merge_with(BlockType block_type) {
    switch (block_type) {
    case BlockType::Dirt:  return std::nullopt;
    case BlockType::Grass: return BlockType::Dirt;
    case BlockType::Stone: return BlockType::Dirt;
    case BlockType::Wood:  return BlockType::Dirt;
    }
}

inline constexpr static bool block_merges_with(BlockType block, BlockType other) {
    const std::optional<BlockType> this_merge = block_merge_with(block);
    const std::optional<BlockType> other_merge = block_merge_with(other);

    if (other_merge == block) return true;
    if (this_merge == other) return true;
    if (this_merge.has_value() && other_merge.has_value() && this_merge == other_merge) return true;

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

inline constexpr static TextureAsset block_texture_asset(BlockType block_type) {
    switch (block_type) {
    case BlockType::Dirt: return TextureAsset::Tiles0;
    case BlockType::Stone: return TextureAsset::Tiles1;
    case BlockType::Grass: return TextureAsset::Tiles2;
    case BlockType::Wood: return TextureAsset::Tiles30;
    }
}

struct Block {
    TextureAtlasPos atlas_pos;
    int16_t hp;
    BlockType type;
    uint8_t variant;
    uint8_t merge_id = 0xFF;
    bool is_merged = false;

    Block(BlockType block_type) :
        atlas_pos(),
        hp(block_hp(block_type)),
        type(block_type),
        variant(static_cast<uint8_t>(rand() % 3)) {}
};

#endif