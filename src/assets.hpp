#ifndef ASSETS_HPP
#define ASSETS_HPP

#pragma once

#include <LLGL/Sampler.h>
#include <LLGL/Utils/VertexFormat.h>
#include <glm/glm.hpp>

#include "types/texture_atlas.hpp"
#include "types/texture.hpp"
#include "types/shader_pipeline.hpp"
#include "types/font.hpp"

enum class TextureAsset : uint8_t {
    Stub = 0,
    Tiles,
    Walls,
    Particles,

    PlayerHead,
    PlayerHair,
    PlayerChest,
    PlayerLegs,
    PlayerLeftShoulder,
    PlayerLeftHand,
    PlayerRightArm,
    PlayerLeftEye,
    PlayerRightEye,

    UiCursorForeground,
    UiCursorBackground,
    UiInventoryBackground,
    UiInventorySelected,
    UiInventoryHotbar,

    Background0,
    Background7,
    Background55,
    Background74,
    Background77,
    Background78,
    Background90,
    Background91,
    Background92,
    Background93,
    Background112,
    Background114,

    TileCracks,
    Tiles0,
    Tiles1,
    Tiles2,
    Tiles30
};

enum class ShaderAsset : uint8_t {
    TilemapShader = 0,
    SpriteShader,
    NinepatchShader,
    PostProcessShader,
    FontShader,
    BackgroundShader,
    ParticleShader,
};

enum class ComputeShaderAsset : uint8_t {
    ParticleComputeTransformShader = 0
};

enum class FontAsset : uint8_t {
    AndyBold = 0,
    AndyRegular
};

enum class VertexFormatAsset : uint8_t {
    SpriteVertex = 0,
    SpriteInstance,
    TilemapVertex,
    TilemapInstance,
    FontVertex,
    FontInstance,
    BackgroundVertex,
    ParticleVertex,
    ParticleInstance,
    PostProcessVertex,
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
    void InitVertexFormats();

    void DestroyTextures();
    void DestroyShaders();
    void DestroySamplers();
    void DestroyFonts();

    const Texture& GetTexture(TextureAsset key);
    const TextureAtlas& GetTextureAtlas(TextureAsset key);
    const Font& GetFont(FontAsset key);
    const Texture& GetItemTexture(size_t index);
    const ShaderPipeline& GetShader(ShaderAsset key);
    LLGL::Shader* GetComputeShader(ComputeShaderAsset key);
    LLGL::Sampler& GetSampler(size_t index);
    const LLGL::VertexFormat& GetVertexFormat(VertexFormatAsset key);
};

#endif