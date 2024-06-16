#pragma once

#ifndef TERRARIA_ASSETS_HPP
#define TERRARIA_ASSETS_HPP

#include <LLGL/LLGL.h>
#include <memory>
#include <glm/glm.hpp>
#include <unordered_map>

#include "renderer/defines.h"
#include "types/block.hpp"
#include "types/wall.hpp"
#include "types/texture_atlas.hpp"
#include "types/texture_descriptor.hpp"

enum AssetKey : uint8_t {
    TextureTiles,
    TextureWalls,
    TextureParticles,

    TexturePlayerHead,
    TexturePlayerHair,
    TexturePlayerChest,
    TexturePlayerFeet,
    TexturePlayerLeftShoulder,
    TexturePlayerLeftHand,
    TexturePlayerRightArm,
    TexturePlayerLeftEye,
    TexturePlayerRightEye,

    TextureUiCursorForeground,
    TextureUiCursorBackground,
    TextureUiInventoryBackground,
    TextureUiInventorySelected,
    TextureUiInventoryHotbar,
};

enum FontKey : uint8_t {
    AndyBold,
    AndyRegular
};

struct AssetTextureAtlas {
    uint32_t rows;
    uint32_t columns;
    glm::uvec2 tile_size;
    glm::uvec2 padding = glm::uvec2(0);
    glm::uvec2 offset = glm::uvec2(0);

    AssetTextureAtlas(uint32_t columns, uint32_t rows, glm::uvec2 tile_size, glm::uvec2 padding = glm::uvec2(0), glm::uvec2 offset = glm::uvec2(0)) :
        columns(columns),
        rows(rows),
        tile_size(tile_size),
        padding(padding),
        offset(offset) {}
};

struct AssetTexture {
    std::string path;
    TextureDescriptor descriptor;

    AssetTexture(std::string path, TextureDescriptor descriptor = {}) :
        path(path),
        descriptor(descriptor) {}
};

static const std::pair<AssetKey, AssetTexture> TEXTURE_ASSETS[] = {
    { AssetKey::TexturePlayerHair,         AssetTexture("assets/sprites/player/Player_Hair_1.png") },
    { AssetKey::TexturePlayerHead,         AssetTexture("assets/sprites/player/Player_0_0.png") },
    { AssetKey::TexturePlayerChest,        AssetTexture("assets/sprites/player/Player_Body.png") },
    { AssetKey::TexturePlayerFeet,         AssetTexture("assets/sprites/player/Player_0_11.png") },
    { AssetKey::TexturePlayerLeftHand,     AssetTexture("assets/sprites/player/Player_Left_Hand.png") },
    { AssetKey::TexturePlayerLeftShoulder, AssetTexture("assets/sprites/player/Player_Left_Shoulder.png") },
    { AssetKey::TexturePlayerRightArm,     AssetTexture("assets/sprites/player/Player_Right_Arm.png") },
    { AssetKey::TexturePlayerLeftEye,      AssetTexture("assets/sprites/player/Player_0_1.png") },
    { AssetKey::TexturePlayerRightEye,     AssetTexture("assets/sprites/player/Player_0_2.png") },

    { AssetKey::TextureUiCursorForeground,    AssetTexture("assets/sprites/ui/Cursor_0.png", { .sampler = TextureSampler::Linear }) },
    { AssetKey::TextureUiCursorBackground,    AssetTexture("assets/sprites/ui/Cursor_11.png", { .sampler = TextureSampler::Linear }) },
    { AssetKey::TextureUiInventoryBackground, AssetTexture("assets/sprites/ui/Inventory_Back.png", { .sampler = TextureSampler::Nearest }) },
    { AssetKey::TextureUiInventorySelected,   AssetTexture("assets/sprites/ui/Inventory_Back14.png", { .sampler = TextureSampler::Nearest }) },
    { AssetKey::TextureUiInventoryHotbar,     AssetTexture("assets/sprites/ui/Inventory_Back9.png", { .sampler = TextureSampler::Nearest }) },

    { AssetKey::TextureParticles, AssetTexture("assets/sprites/Particles.png") }
};

constexpr uint32_t PARTICLES_ATLAS_COLUMNS = 100;

static const std::pair<AssetKey, AssetTextureAtlas> TEXTURE_ATLAS_ASSETS[] = {
    { AssetKey::TexturePlayerHair,         AssetTextureAtlas(1, 14, glm::uvec2(40, 64)) },
    { AssetKey::TexturePlayerHead,         AssetTextureAtlas(1, 14, glm::uvec2(40, 48)) },
    { AssetKey::TexturePlayerChest,        AssetTextureAtlas(1, 14, glm::uvec2(32, 64), glm::uvec2(8, 0)) },
    { AssetKey::TexturePlayerFeet,         AssetTextureAtlas(1, 19, glm::uvec2(40, 64)) },
    { AssetKey::TexturePlayerLeftHand,     AssetTextureAtlas(27, 1, glm::uvec2(32, 64)) },
    { AssetKey::TexturePlayerLeftShoulder, AssetTextureAtlas(27, 1, glm::uvec2(32, 64)) },
    { AssetKey::TexturePlayerRightArm,     AssetTextureAtlas(18, 1, glm::uvec2(32, 80)) },
    { AssetKey::TexturePlayerLeftEye,      AssetTextureAtlas(1, 20, glm::uvec2(40, 64)) },
    { AssetKey::TexturePlayerRightEye,     AssetTextureAtlas(1, 20, glm::uvec2(40, 64)) },
    { AssetKey::TextureParticles,          AssetTextureAtlas(PARTICLES_ATLAS_COLUMNS, 12, glm::uvec2(8), glm::uvec2(2)) }
};

static const std::pair<uint16_t, std::string> BLOCK_ASSETS[] = {
    { static_cast<uint16_t>(BlockType::Dirt), "assets/sprites/tiles/Tiles_0.png" },
    { static_cast<uint16_t>(BlockType::Stone), "assets/sprites/tiles/Tiles_1.png" },
    { static_cast<uint16_t>(BlockType::Grass), "assets/sprites/tiles/Tiles_2.png" },
    { static_cast<uint16_t>(BlockType::Wood), "assets/sprites/tiles/Tiles_30.png" },
};

static const std::pair<uint16_t, std::string> WALL_ASSETS[] = {
    { static_cast<uint16_t>(WallType::DirtWall), "assets/sprites/walls/Wall_2.png" },
    { static_cast<uint16_t>(WallType::StoneWall), "assets/sprites/walls/Wall_1.png" },
};

static const std::pair<uint16_t, std::string> ITEM_ASSETS[] = {
    { 2, "assets/sprites/items/Item_2.png" },
    { 3, "assets/sprites/items/Item_3.png" },
    { 9, "assets/sprites/items/Item_9.png" },
    { 26, "assets/sprites/items/Item_26.png" },
    { 30, "assets/sprites/items/Item_30.png" },
    { 62, "assets/sprites/items/Item_62.png" },
    { 3505, "assets/sprites/items/Item_3505.png" },
    { 3506, "assets/sprites/items/Item_3506.png" },
    { 3509, "assets/sprites/items/Item_3509.png" },
};

static const std::pair<FontKey, std::string> FONT_ASSETS[] = {
    { FontKey::AndyBold, "assets/fonts/andy_bold.ttf" },
    { FontKey::AndyRegular, "assets/fonts/andy_regular.otf" },
};

struct AssetsState {
    std::unordered_map<uint16_t, LLGL::Texture*> items;
    std::unordered_map<AssetKey, LLGL::Texture*> textures;
    std::unordered_map<AssetKey, TextureAtlas> textures_atlases;
    // std::unordered_map<FontKey, Font> fonts;
};

static AssetsState assets_state;

namespace Assets {
    bool load();
};

#endif