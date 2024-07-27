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
#include "types/texture.hpp"

enum AssetKey : uint8_t {
    TextureStub,
    TextureTiles,
    TextureWalls,
    TextureParticles,

    TexturePlayerHead,
    TexturePlayerHair,
    TexturePlayerChest,
    TexturePlayerLegs,
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
    int sampler;

    AssetTexture(std::string path, int sampler = TextureSampler::Nearest) :
        path(path),
        sampler(sampler) {}
};

constexpr uint32_t PARTICLES_ATLAS_COLUMNS = 100;

namespace Assets {
    bool Load();
    void DestroyTextures();
    const Texture& GetTexture(AssetKey key);
    const TextureAtlas& GetTextureAtlas(AssetKey key);
};

#endif