#ifndef ASSETS_HPP
#define ASSETS_HPP

#pragma once

#include <LLGL/Sampler.h>
#include <glm/glm.hpp>

#include "types/texture_atlas.hpp"
#include "types/texture.hpp"
#include "types/shader_pipeline.hpp"
#include "types/font.hpp"

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
    PostProcessShader,
    FontShader,
};

enum class FontKey : uint8_t {
    AndyBold = 0,
    AndyRegular
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
    bool LoadFonts();
    bool InitSamplers();

    void DestroyTextures();
    void DestroyShaders();
    void DestroySamplers();
    void DestroyFonts();

    const Texture& GetTexture(AssetKey key);
    const TextureAtlas& GetTextureAtlas(AssetKey key);
    const Font& GetFont(FontKey key);
    const Texture& GetItemTexture(size_t index);
    const ShaderPipeline& GetShader(ShaderAssetKey key);
    LLGL::Sampler& GetSampler(size_t index);
};

#endif