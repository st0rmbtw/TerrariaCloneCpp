#ifndef TYPES_BLOCK_HPP
#define TYPES_BLOCK_HPP

#pragma once

#include <cstdint>
#include <optional>

#include "texture_atlas_pos.hpp"
#include "neighbors.hpp"
#include "tool_flags.hpp"

#include "../assets.hpp"
#include "../utils.hpp"

namespace TileType {
    enum : uint8_t {
        Block = 0,
        Wall,
        Torch,
        Tree,
        TreeBranch,
        TreeCrown,
        Count,
    };
}

enum class BlockType : uint8_t {
    Dirt = 0,
    Stone = 1,
    Grass = 2,
    Torch = 4,
    Tree = 5,
    Wood = 30,
};

enum class TileTextureType : uint8_t {
    Dirt = 0,
    Stone,
    Grass,
    Torch,
    TreeTrunk,
    TreeCrown,
    TreeBranch,
    Wood
};

enum class TreeType : uint8_t {
    Forest = 0
};

enum class TreeFrameType : uint8_t {
    // A trunk
    Trunk = 0,

    TrunkBranchLeft,
    TrunkBranchRight,
    TrunkBranchBoth,

    TrunkHollowLeft,
    TrunkHollowRight,

    TrunkBranchCollarLeft,
    TrunkBranchCollarRight,

    // A left root
    RootLeft,
    // A right root
    RootRight,
    // A center base when there is only a left root
    BaseLeft,
    // A center base when there is only a right root
    BaseRight,
    // A center base when there are both a left and a right roots
    BaseBoth,
    // A left side branch with no leaves 
    BranchLeftBare,
    // A right side branch with no leaves
    BranchRightBare,
    // A left side branch with leaves
    BranchLeftLeaves,
    // A right side branch with leaves
    BranchRightLeaves,
    // A top of a tree with no leaves
    TopBare,
    // A top of a tree with leaves
    TopLeaves,
    // A jagged top of a tree with no leaves
    TopBareJagged,

    // A stump when there is only a left root
    StumpRootLeft,
    // A stump when there is only a right root
    StumpRootRight,
    // A stump when there are both a left root and a right roots
    StumpRootBoth,
};

struct TreeData {
    TreeType type;
    TreeFrameType frame;
};

union BlockData {
    TreeData tree;
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

inline constexpr int16_t block_hp(BlockType tile_type) {
    switch (tile_type) {
    case BlockType::Torch: return 1;
    case BlockType::Dirt:  case BlockType::Grass: return 50;
    case BlockType::Stone: case BlockType::Wood:  return 100;
    case BlockType::Tree:  return 500;
    }
}

struct Block {
    TextureAtlasPos atlas_pos;
    int16_t hp;
    BlockType type;
    BlockData data;
    uint8_t variant;
    uint8_t merge_id = 0xFF;
    bool is_merged = false;

    explicit Block(BlockType tile_type) :
        atlas_pos(),
        hp(block_hp(tile_type)),
        type(tile_type),
        variant(static_cast<uint8_t>(rand() % 3)) {}

    static Block Tree(TreeType type, TreeFrameType frame) {
        Block tile = Block(BlockType::Tree);
        tile.data.tree.type = type;
        tile.data.tree.frame = frame;
        return tile;
    }
};

struct BlockTypeWithData {
    constexpr BlockTypeWithData(BlockType block_type) :
        type(block_type) {}

    constexpr BlockTypeWithData(BlockType block_type, BlockData block_data) :
        type(block_type),
        data(block_data) {}

    constexpr BlockTypeWithData(const Block& block) :
        type(block.type),
        data(block.data) {}

    BlockType type;
    BlockData data;
};

inline constexpr uint8_t tile_type(BlockTypeWithData tile) {
    switch (tile.type) {
    case BlockType::Dirt: 
    case BlockType::Stone:
    case BlockType::Grass:
    case BlockType::Wood:
        return TileType::Block;

    case BlockType::Torch:
        return TileType::Torch;

    case BlockType::Tree: switch (tile.data.tree.frame) {
        case TreeFrameType::BranchLeftLeaves:
        case TreeFrameType::BranchRightLeaves:
            return TileType::TreeBranch;
        
        case TreeFrameType::TopLeaves:
            return TileType::TreeCrown;

        default:
            return TileType::Tree;
        }
    }
}

inline constexpr TileTextureType tile_texture_type(BlockTypeWithData tile) {
    switch (tile.type) {
    case BlockType::Dirt:  return TileTextureType::Dirt;
    case BlockType::Stone: return TileTextureType::Stone;
    case BlockType::Grass: return TileTextureType::Grass;
    case BlockType::Torch: return TileTextureType::Torch;
    case BlockType::Tree: switch (tile.data.tree.frame) {
        case TreeFrameType::BranchLeftLeaves:
        case TreeFrameType::BranchRightLeaves:
            return TileTextureType::TreeBranch;
        
        case TreeFrameType::TopLeaves:
            return TileTextureType::TreeCrown;

        default:
            return TileTextureType::TreeTrunk;
    }
    case BlockType::Wood:  return TileTextureType::Wood;
    }
}

inline constexpr TextureAsset block_texture_asset(BlockTypeWithData block) {
    switch (block.type) {
    case BlockType::Dirt:  return TextureAsset::Tiles0;
    case BlockType::Stone: return TextureAsset::Tiles1;
    case BlockType::Grass: return TextureAsset::Tiles2;
    case BlockType::Torch: return TextureAsset::Tiles4;
    case BlockType::Tree: switch (block.data.tree.frame) {
        case TreeFrameType::BranchLeftLeaves:
        case TreeFrameType::BranchRightLeaves:
            return TextureAsset::TreeBranches;
        
        case TreeFrameType::TopLeaves:
            return TextureAsset::TreeTops;

        default:
            return TextureAsset::Tiles5;
        }
    case BlockType::Wood:  return TextureAsset::Tiles30;
    }
}

inline constexpr bool tile_is_block(BlockTypeWithData block) {
    switch (block.type) {
    case BlockType::Torch: return false;
    case BlockType::Tree: switch (block.data.tree.frame) {
        case TreeFrameType::BranchLeftLeaves:
        case TreeFrameType::BranchRightLeaves:
        case TreeFrameType::TopLeaves:
            return false;
        default:
            return true;
        }
    default:
        return true;
    }
}

inline constexpr uint8_t tree_is_trunk(TreeFrameType frame) {
    switch (frame) {
    case TreeFrameType::Trunk:
    case TreeFrameType::TrunkHollowLeft:
    case TreeFrameType::TrunkHollowRight:
    case TreeFrameType::TrunkBranchCollarLeft:
    case TreeFrameType::TrunkBranchCollarRight:
    case TreeFrameType::RootLeft:
    case TreeFrameType::RootRight:
    case TreeFrameType::BaseLeft:
    case TreeFrameType::BaseRight:
    case TreeFrameType::BaseBoth:
    case TreeFrameType::StumpRootLeft:
    case TreeFrameType::StumpRootRight:
    case TreeFrameType::StumpRootBoth:
    case TreeFrameType::TopBare:
    case TreeFrameType::TopBareJagged:
        return true;
    default:
        return false;
    }
}

inline constexpr uint8_t block_required_tool(BlockType tile_type) {
    switch (tile_type) {
    case BlockType::Dirt:
    case BlockType::Stone:
    case BlockType::Grass:
    case BlockType::Wood:
        return ToolFlags::Pickaxe;

    case BlockType::Torch:
        return ToolFlags::Any;

    case BlockType::Tree:
        return ToolFlags::Axe;
    }
}

inline constexpr bool block_is_stone(BlockType tile_type) {
    switch (tile_type) {
    case BlockType::Stone: return true;
    default: return false;
    }
}

inline constexpr std::optional<BlockType> block_merge_with(BlockType tile_type) {
    switch (tile_type) {
    case BlockType::Grass: return BlockType::Dirt;
    case BlockType::Stone: return BlockType::Dirt;
    case BlockType::Wood:  return BlockType::Dirt;
    default: return std::nullopt;
    }
}

inline constexpr bool block_merges_with(BlockType block, BlockType other) {
    if (!tile_is_block(block)) return false;

    const std::optional<BlockType> this_merge = block_merge_with(block);
    const std::optional<BlockType> other_merge = block_merge_with(other);

    if (other_merge == block) return true;
    if (this_merge == other) return true;
    if (this_merge.has_value() && other_merge.has_value() && this_merge == other_merge) return true;

    return false;
}

inline constexpr const char* block_type_name(BlockType tile_type) {
    switch (tile_type) {
    case BlockType::Dirt:  return "Dirt";
    case BlockType::Grass: return "Grass";
    case BlockType::Stone: return "Stone";
    case BlockType::Wood:  return "Wood";
    case BlockType::Torch: return "Torch";
    case BlockType::Tree:  return "Tree";
    }
}

inline constexpr bool block_dusty(BlockType tile_type) {
    switch (tile_type) {
    case BlockType::Dirt:  return true;
    default: return false;
    }
}

inline constexpr AnchorData block_anchor(BlockType tile_type) {
    switch (tile_type) {
    case BlockType::Torch: return AnchorData {
        .left = AnchorType::Default | AnchorType::Tree,
        .right = AnchorType::Default | AnchorType::Tree,
        .bottom = AnchorType::Default,
        .top = AnchorType::None
    };
    default: return AnchorData();
    }
}

inline constexpr bool block_is_solid(std::optional<BlockType> tile_type) {
    if (!tile_type.has_value()) return false;

    switch (tile_type.value()) {
    case BlockType::Torch:
    case BlockType::Tree: return false;
    default: return true;
    }
}

inline constexpr bool check_anchor_horizontal(std::optional<BlockType> tile, AnchorType::Type anchor) {
    if (anchor == AnchorType::None && tile.has_value()) return false;
    if (anchor != AnchorType::None && !tile.has_value()) return false;

    if (BITFLAG_CHECK(anchor, AnchorType::SolidTile) && !block_is_solid(tile)) return false;
    if (BITFLAG_CHECK(anchor, AnchorType::SolidSide) && !block_is_solid(tile)) return false;
    return true;
}

inline constexpr bool check_anchor_vertical(std::optional<BlockType> tile, AnchorType::Type anchor) {
    if (BITFLAG_CHECK(anchor, AnchorType::SolidTile) && block_is_solid(tile)) return true;
    return false;
}

inline constexpr bool check_anchor_data(AnchorData anchor, const Neighbors<BlockType>& neighbors, bool has_wall) {
    if (check_anchor_horizontal(neighbors.left, anchor.left)) return true;
    if (check_anchor_horizontal(neighbors.right, anchor.right)) return true;
    if (check_anchor_vertical(neighbors.top, anchor.top)) return true;
    if (check_anchor_vertical(neighbors.bottom, anchor.bottom)) return true;
    if (anchor.wall && has_wall) return true;

    return false;
}

inline constexpr std::optional<glm::vec3> block_light(std::optional<BlockType> tile_type) {
    if (!tile_type.has_value()) return std::nullopt;

    switch (tile_type.value()) {
    case BlockType::Torch: return glm::vec3(1.0f, 0.95f, 0.8f);
    default: return std::nullopt;
    }
}

#endif