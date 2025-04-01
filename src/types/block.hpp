#ifndef TYPES_BLOCK_HPP
#define TYPES_BLOCK_HPP

#pragma once

#include <stdint.h>
#include <optional>

#include "texture_atlas_pos.hpp"
#include "neighbors.hpp"
#include "../assets.hpp"
#include "../utils.hpp"

enum class TileType : uint8_t {
    Dirt = 0,
    Stone = 1,
    Grass = 2,
    Torch = 4,
    Wood = 30,
};

namespace AnchorType {
    using Type = uint8_t;
    enum : Type { 
        None = 0,
        SolidSide = 1 << 0,
        SolidTile = 1 << 1,
        Tree = 1 << 2
    };

    static constexpr Type Default = SolidTile | SolidSide;
}

struct AnchorData {
    AnchorType::Type left = AnchorType::Default;
    AnchorType::Type right = AnchorType::Default;
    AnchorType::Type bottom = AnchorType::Default;
    AnchorType::Type top = AnchorType::Default;
    bool wall = true;
};

inline constexpr static bool tile_is_block(TileType tile_type) {
    switch (tile_type) {
    case TileType::Torch: return false;
    default: return true;
    }
}

inline constexpr static bool tile_is_stone(TileType tile_type) {
    switch (tile_type) {
    case TileType::Stone: return true;
    default: return false;
    }
}

inline constexpr static bool tile_hand_destroy(TileType tile_type) {
    switch (tile_type) {
    case TileType::Torch: return true;
    default: return false;
    }
}

inline constexpr static int16_t tile_hp(TileType tile_type) {
    switch (tile_type) {
    case TileType::Torch: return 1;
    case TileType::Dirt:  case TileType::Grass: return 50;
    case TileType::Stone: case TileType::Wood:  return 100;
    }
}

inline constexpr static std::optional<TileType> tile_merge_with(TileType tile_type) {
    switch (tile_type) {
    case TileType::Grass: return TileType::Dirt;
    case TileType::Stone: return TileType::Dirt;
    case TileType::Wood:  return TileType::Dirt;
    default: return std::nullopt;
    }
}

inline constexpr static bool tile_merges_with(TileType block, TileType other) {
    if (!tile_is_block(block)) return false;

    const std::optional<TileType> this_merge = tile_merge_with(block);
    const std::optional<TileType> other_merge = tile_merge_with(other);

    if (other_merge == block) return true;
    if (this_merge == other) return true;
    if (this_merge.has_value() && other_merge.has_value() && this_merge == other_merge) return true;

    return false;
}

inline constexpr static const char* block_type_name(TileType tile_type) {
    switch (tile_type) {
    case TileType::Dirt:  return "Dirt";
    case TileType::Grass: return "Grass";
    case TileType::Stone: return "Stone";
    case TileType::Wood:  return "Wood";
    case TileType::Torch: return "Torch";
    }
}

inline constexpr static bool tile_dusty(TileType tile_type) {
    switch (tile_type) {
    case TileType::Dirt:  return true;
    default: return false;
    }
}

inline constexpr static TextureAsset tile_texture_asset(TileType tile_type) {
    switch (tile_type) {
    case TileType::Dirt: return TextureAsset::Tiles0;
    case TileType::Stone: return TextureAsset::Tiles1;
    case TileType::Grass: return TextureAsset::Tiles2;
    case TileType::Torch: return TextureAsset::Tiles4;
    case TileType::Wood: return TextureAsset::Tiles30;
    }
}

inline constexpr static AnchorData tile_anchor(TileType tile_type) {
    switch (tile_type) {
    case TileType::Torch: return AnchorData {
        .left = AnchorType::Default | AnchorType::Tree,
        .right = AnchorType::Default | AnchorType::Tree,
        .bottom = AnchorType::Default,
        .top = AnchorType::None
    };
    default: return AnchorData();
    }
}

inline constexpr static bool tile_is_solid(std::optional<TileType> tile_type) {
    if (!tile_type.has_value()) return false;

    switch (tile_type.value()) {
    case TileType::Torch: return false;
    default: return true;
    }
}

inline constexpr static bool check_anchor_horizontal(std::optional<TileType> tile, AnchorType::Type anchor) {
    if (anchor == AnchorType::None && tile.has_value()) return false;
    if (anchor != AnchorType::None && !tile.has_value()) return false;

    if (BITFLAG_CHECK(anchor, AnchorType::SolidTile) && !tile_is_solid(tile)) return false;
    if (BITFLAG_CHECK(anchor, AnchorType::SolidSide) && !tile_is_solid(tile)) return false;
    return true;
}

inline constexpr static bool check_anchor_vertical(std::optional<TileType> tile, AnchorType::Type anchor) {
    if (BITFLAG_CHECK(anchor, AnchorType::SolidTile) && tile_is_solid(tile)) return true;
    return false;
}

inline constexpr static bool check_anchor_data(AnchorData anchor, const Neighbors<TileType>& neighbors, bool has_wall) {
    if (check_anchor_horizontal(neighbors.left, anchor.left)) return true;
    if (check_anchor_horizontal(neighbors.right, anchor.right)) return true;
    if (check_anchor_vertical(neighbors.top, anchor.top)) return true;
    if (check_anchor_vertical(neighbors.bottom, anchor.bottom)) return true;
    if (anchor.wall && has_wall) return true;

    return false;
}

inline constexpr static std::optional<glm::vec3> tile_light(std::optional<TileType> tile_type) {
    if (!tile_type.has_value()) return std::nullopt;

    switch (tile_type.value()) {
    case TileType::Torch: return glm::vec3(1.0f, 0.95f, 0.8f);
    default: return std::nullopt;
    }
}

struct Tile {
    TextureAtlasPos atlas_pos;
    int16_t hp;
    TileType type;
    uint8_t variant;
    uint8_t merge_id = 0xFF;
    bool is_merged = false;

    Tile(TileType tile_type) :
        atlas_pos(),
        hp(tile_hp(tile_type)),
        type(tile_type),
        variant(static_cast<uint8_t>(rand() % 3)) {}
};

#endif