#ifndef ASSETS_HPP
#define ASSETS_HPP

#pragma once

#include <LLGL/Sampler.h>
#include <LLGL/Utils/VertexFormat.h>
#include <glm/glm.hpp>

#include <SGE/types/texture_atlas.hpp>
#include <SGE/types/texture.hpp>
#include <SGE/types/shader_pipeline.hpp>
#include <SGE/types/font.hpp>
#include <SGE/types/sampler.hpp>
#include <SGE/types/shader_def.hpp>

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
    Tiles4,
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
    PostProcessShader,
    BackgroundShader,
    ParticleShader,
    StaticLightMapShader,
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

constexpr uint32_t PARTICLES_ATLAS_COLUMNS = 100;

namespace Assets {
    bool Load();
    bool LoadShaders(const std::vector<sge::ShaderDef>& shader_defs);
    bool LoadFonts();
    void InitVertexFormats();

    void DestroyTextures();
    void DestroyShaders();
    void DestroySamplers();
    void DestroyFonts();

    const sge::Texture& GetTexture(TextureAsset key);
    const sge::TextureAtlas& GetTextureAtlas(TextureAsset key);
    const sge::Font& GetFont(FontAsset key);
    const sge::Texture& GetItemTexture(uint16_t index);
    const sge::ShaderPipeline& GetShader(ShaderAsset key);
    LLGL::Shader* GetComputeShader(ComputeShaderAsset key);
    const sge::Sampler& GetSampler(size_t index);
    const LLGL::VertexFormat& GetVertexFormat(VertexFormatAsset key);
};

#endif