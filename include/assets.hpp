#ifndef ASSETS_HPP
#define ASSETS_HPP

#pragma once

#include <LLGL/Sampler.h>
#include <memory>
#include <glm/glm.hpp>
#include <unordered_map>

#include "renderer/defines.h"
#include "types/block.hpp"
#include "types/wall.hpp"
#include "types/texture_atlas.hpp"
#include "types/texture.hpp"
#include "types/shader_pipeline.hpp"

enum class AssetKey : uint8_t {
    TextureStub = 0,
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

enum class ShaderAssetKey : uint8_t {
    TilemapShader = 0,
    SpriteShader,
    NinepatchShader,
    PostprocessShader,
    FontShader,
};

enum class FontKey : uint8_t {
    AndyBold = 0,
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

    explicit AssetTexture(std::string path, int sampler = TextureSampler::Nearest) :
        path(std::move(path)),
        sampler(sampler) {}
};

struct ShaderDef {
    std::string name;
    std::string value;
    
    ShaderDef(std::string name, std::string value) :
        name(std::move(name)),
        value(std::move(value)) {}
};

constexpr uint32_t PARTICLES_ATLAS_COLUMNS = 100;

namespace Assets {
    bool Load();
    bool LoadShaders(const std::vector<ShaderDef>& shader_defs);
    bool InitSamplers();

    void DestroyTextures();
    void DestroyShaders();
    void DestroySamplers();

    const Texture& GetTexture(AssetKey key);
    const TextureAtlas& GetTextureAtlas(AssetKey key);
    const Texture& GetItemTexture(size_t index);
    const ShaderPipeline& GetShader(ShaderAssetKey key);
    LLGL::Sampler& GetSampler(size_t index);
};

#endif