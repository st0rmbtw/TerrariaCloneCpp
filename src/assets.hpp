#pragma once

#ifndef ASSETS_HPP_
#define ASSETS_HPP_

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
    UiPanelBackground,
    UiPanelBorder,

    TileCracks,
    Tiles0,
    Tiles1,
    Tiles2,
    Tiles4,
    Tiles5,
    Tiles30,

    TreeTops,
    TreeBranches,

    Flames0
};

enum class BackgroundAsset : uint8_t {
    Background0 = 0,
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
    Count
};

enum class ShaderAsset : uint8_t {
    TilemapShader = 0,
    PostProcessShader,
    BackgroundShader,
    ParticleShader,
    StaticLightMapShader,
    FontShader
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
    TilemapVertex = 0,
    TilemapInstance,
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
    const sge::Texture& GetItemTexture(uint16_t id);
    const sge::ShaderPipeline& GetShader(ShaderAsset key);
    LLGL::Shader* GetComputeShader(ComputeShaderAsset key);
    const sge::Sampler& GetSampler(size_t index);
    const LLGL::VertexFormat& GetVertexFormat(VertexFormatAsset key);
};

#endif