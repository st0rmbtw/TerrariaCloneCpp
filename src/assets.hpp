#ifndef ASSETS_HPP
#define ASSETS_HPP

#pragma once

#include <LLGL/Sampler.h>
#include <LLGL/Utils/VertexFormat.h>
#include <glm/glm.hpp>

#include "engine/types/texture_atlas.hpp"
#include "engine/types/texture.hpp"
#include "engine/types/shader_pipeline.hpp"
#include "engine/types/font.hpp"
#include "engine/types/sampler.hpp"

enum class TextureAsset : uint8_t {
    Stub = 0,
    Tiles,
    Walls,
    Particles,
    Backgrounds,

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

    TileCracks,
    Tiles0,
    Tiles1,
    Tiles2,
    Tiles30
};

enum class BackgroundAsset : uint16_t {
    Background0 = 0,
    Background7 = 7,
    Background55 = 55,
    Background74 = 74,
    Background77 = 77,
    Background78 = 78,
    Background90 = 90,
    Background91 = 91,
    Background92 = 92,
    Background93 = 93,
    Background112 = 112,
    Background114 = 114,
    Count
};

enum class ShaderAsset : uint8_t {
    TilemapShader = 0,
    SpriteShader,
    NinePatchShader,
    PostProcessShader,
    FontShader,
    BackgroundShader,
    ParticleShader,
    StaticLightMapShader
};

enum class ComputeShaderAsset : uint8_t {
    ParticleComputeTransformShader = 0,
    LightSetLightSources,
    LightVertical,
    LightHorizontal,
};

enum class FontAsset : uint8_t {
    AndyBold = 0,
    AndyRegular
};

enum class VertexFormatAsset : uint8_t {
    SpriteVertex = 0,
    SpriteInstance,
    NinePatchVertex,
    NinePatchInstance,
    TilemapVertex,
    TilemapInstance,
    FontVertex,
    FontInstance,
    BackgroundVertex,
    BackgroundInstance,
    ParticleVertex,
    ParticleInstance,
    PostProcessVertex,
    StaticLightMapVertex,
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
    const Texture& GetItemTexture(uint16_t index);
    const ShaderPipeline& GetShader(ShaderAsset key);
    LLGL::Shader* GetComputeShader(ComputeShaderAsset key);
    const Sampler& GetSampler(size_t index);
    const LLGL::VertexFormat& GetVertexFormat(VertexFormatAsset key);

    inline const Sampler& GetSampler(const Texture& texture) {
        return GetSampler(texture.sampler());
    }
};

#endif