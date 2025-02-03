#include "assets.hpp"

#include <array>
#include <fstream>
#include <filesystem>

#include <LLGL/Utils/VertexFormat.h>
#include <LLGL/Shader.h>
#include <LLGL/ShaderFlags.h>
#include <LLGL/Format.h>
#include <LLGL/VertexAttribute.h>
#include <LLGL/TextureFlags.h>
#include <STB/stb_image.h>
#include <freetype/freetype.h>

#include "engine/engine.hpp"
#include "engine/types/shader_pipeline.hpp"
#include "engine/types/texture.hpp"
#include "engine/log.hpp"
#include "engine/utils.hpp"
#include "renderer/types.hpp"
#include "types/block.hpp"
#include "types/wall.hpp"

#include "utils.hpp"

namespace fs = std::filesystem;

struct AssetTextureAtlas {
    uint32_t rows;
    uint32_t columns;
    glm::uvec2 tile_size;
    glm::uvec2 padding = glm::uvec2(0);
    glm::uvec2 offset = glm::uvec2(0);

    AssetTextureAtlas(uint32_t columns, uint32_t rows, glm::uvec2 tile_size, glm::uvec2 padding = glm::uvec2(0), glm::uvec2 offset = glm::uvec2(0)) :
        rows(rows),
        columns(columns),
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

namespace ShaderStages {
    static constexpr uint8_t Vertex = 1 << 0;
    static constexpr uint8_t Fragment = 1 << 1;
    static constexpr uint8_t Geometry = 1 << 2;
    static constexpr uint8_t Compute = 1 << 3;
}

struct AssetShader {
    std::string file_name;
    uint8_t stages;
    std::vector<VertexFormatAsset> vertex_format_assets;

    explicit AssetShader(std::string file_name, uint8_t stages) :
        file_name(std::move(file_name)),
        stages(stages) {}

    explicit AssetShader(std::string file_name, uint8_t stages, VertexFormatAsset vertex_format) :
        file_name(std::move(file_name)),
        stages(stages),
        vertex_format_assets({vertex_format}) {}

    explicit AssetShader(std::string file_name, uint8_t stages, std::vector<VertexFormatAsset> vertex_formats) :
        file_name(std::move(file_name)),
        stages(stages),
        vertex_format_assets(std::move(vertex_formats)) {}
};

struct AssetComputeShader {
    std::string file_name;
    std::string func_name;
    std::string glsl_file_name;

    explicit AssetComputeShader(std::string file_name, std::string func_name, std::string glsl_file_name = {}) :
        file_name(std::move(file_name)),
        func_name(std::move(func_name)),
        glsl_file_name(std::move(glsl_file_name)) {}
};

static const std::pair<TextureAsset, AssetTexture> TEXTURE_ASSETS[] = {
    { TextureAsset::PlayerHair,         AssetTexture("assets/sprites/player/Player_Hair_1.png") },
    { TextureAsset::PlayerHead,         AssetTexture("assets/sprites/player/Player_0_0.png") },
    { TextureAsset::PlayerChest,        AssetTexture("assets/sprites/player/Player_Body.png") },
    { TextureAsset::PlayerLegs,         AssetTexture("assets/sprites/player/Player_0_11.png") },
    { TextureAsset::PlayerLeftHand,     AssetTexture("assets/sprites/player/Player_Left_Hand.png") },
    { TextureAsset::PlayerLeftShoulder, AssetTexture("assets/sprites/player/Player_Left_Shoulder.png") },
    { TextureAsset::PlayerRightArm,     AssetTexture("assets/sprites/player/Player_Right_Arm.png") },
    { TextureAsset::PlayerLeftEye,      AssetTexture("assets/sprites/player/Player_0_1.png") },
    { TextureAsset::PlayerRightEye,     AssetTexture("assets/sprites/player/Player_0_2.png") },

    { TextureAsset::UiCursorForeground,    AssetTexture("assets/sprites/ui/Cursor_0.png", TextureSampler::Linear) },
    { TextureAsset::UiCursorBackground,    AssetTexture("assets/sprites/ui/Cursor_11.png", TextureSampler::Linear) },
    { TextureAsset::UiInventoryBackground, AssetTexture("assets/sprites/ui/Inventory_Back.png", TextureSampler::Linear) },
    { TextureAsset::UiInventorySelected,   AssetTexture("assets/sprites/ui/Inventory_Back14.png", TextureSampler::Linear) },
    { TextureAsset::UiInventoryHotbar,     AssetTexture("assets/sprites/ui/Inventory_Back9.png", TextureSampler::Linear) },

    { TextureAsset::TileCracks, AssetTexture("assets/sprites/tiles/TileCracks.png", TextureSampler::Nearest) },

    { TextureAsset::Particles, AssetTexture("assets/sprites/Particles.png") }
};

static const std::pair<TextureAsset, AssetTextureAtlas> TEXTURE_ATLAS_ASSETS[] = {
    { TextureAsset::PlayerHair,         AssetTextureAtlas(1, 14, glm::uvec2(40, 64)) },
    { TextureAsset::PlayerHead,         AssetTextureAtlas(1, 14, glm::uvec2(40, 48)) },
    { TextureAsset::PlayerChest,        AssetTextureAtlas(1, 14, glm::uvec2(32, 64), glm::uvec2(8, 0)) },
    { TextureAsset::PlayerLegs,         AssetTextureAtlas(1, 19, glm::uvec2(40, 64)) },
    { TextureAsset::PlayerLeftHand,     AssetTextureAtlas(27, 1, glm::uvec2(32, 64)) },
    { TextureAsset::PlayerLeftShoulder, AssetTextureAtlas(27, 1, glm::uvec2(32, 64)) },
    { TextureAsset::PlayerRightArm,     AssetTextureAtlas(18, 1, glm::uvec2(32, 80)) },
    { TextureAsset::PlayerLeftEye,      AssetTextureAtlas(1, 20, glm::uvec2(40, 64)) },
    { TextureAsset::PlayerRightEye,     AssetTextureAtlas(1, 20, glm::uvec2(40, 64)) },
    { TextureAsset::Particles,          AssetTextureAtlas(PARTICLES_ATLAS_COLUMNS, 12, glm::uvec2(8), glm::uvec2(2)) },

    { TextureAsset::TileCracks,         AssetTextureAtlas(6, 4, glm::uvec2(16)) }
};

#define BLOCK_ASSET(BLOCK_TYPE, TEXTURE_ASSET, PATH) std::make_tuple(static_cast<uint16_t>(BLOCK_TYPE), TEXTURE_ASSET, PATH)
static const std::array BLOCK_ASSETS = {
    BLOCK_ASSET(BlockType::Dirt, TextureAsset::Tiles0, "assets/sprites/tiles/Tiles_0.png"),
    BLOCK_ASSET(BlockType::Stone, TextureAsset::Tiles1, "assets/sprites/tiles/Tiles_1.png"),
    BLOCK_ASSET(BlockType::Grass, TextureAsset::Tiles2, "assets/sprites/tiles/Tiles_2.png"),
    BLOCK_ASSET(BlockType::Wood, TextureAsset::Tiles30, "assets/sprites/tiles/Tiles_30.png"),
};

#define WALL_ASSET(WALL_TYPE, PATH) std::make_tuple(static_cast<uint16_t>(WALL_TYPE), TextureAsset::Stub, PATH)
static const std::array WALL_ASSETS = {
    WALL_ASSET(WallType::DirtWall, "assets/sprites/walls/Wall_2.png"),
    WALL_ASSET(WallType::StoneWall, "assets/sprites/walls/Wall_1.png"),
    WALL_ASSET(WallType::WoodWall, "assets/sprites/walls/Wall_4.png"),
};

#define BACKGROUND_ASSET(BACKGROUND_ASSET, PATH) std::make_tuple(static_cast<uint16_t>(BACKGROUND_ASSET), TextureAsset::Stub, PATH)
static const std::array BACKGROUND_ASSETS = {
    BACKGROUND_ASSET(BackgroundAsset::Background0, "assets/sprites/backgrounds/Background_0.png"),
    BACKGROUND_ASSET(BackgroundAsset::Background7, "assets/sprites/backgrounds/Background_7.png"),
    BACKGROUND_ASSET(BackgroundAsset::Background55, "assets/sprites/backgrounds/Background_55.png"),
    BACKGROUND_ASSET(BackgroundAsset::Background74, "assets/sprites/backgrounds/Background_74.png"),
    BACKGROUND_ASSET(BackgroundAsset::Background77, "assets/sprites/backgrounds/Background_77.png"),
    BACKGROUND_ASSET(BackgroundAsset::Background78, "assets/sprites/backgrounds/Background_78.png"),
    BACKGROUND_ASSET(BackgroundAsset::Background90, "assets/sprites/backgrounds/Background_90.png"),
    BACKGROUND_ASSET(BackgroundAsset::Background91, "assets/sprites/backgrounds/Background_91.png"),
    BACKGROUND_ASSET(BackgroundAsset::Background92, "assets/sprites/backgrounds/Background_92.png"),
    BACKGROUND_ASSET(BackgroundAsset::Background93, "assets/sprites/backgrounds/Background_93.png"),
    BACKGROUND_ASSET(BackgroundAsset::Background112, "assets/sprites/backgrounds/Background_112.png"),
    BACKGROUND_ASSET(BackgroundAsset::Background114, "assets/sprites/backgrounds/Background_114.png"),
};

static const std::pair<uint16_t, std::string> ITEM_ASSETS[] = {
    { 2, "assets/sprites/items/Item_2.png" },
    { 3, "assets/sprites/items/Item_3.png" },
    { 8, "assets/sprites/items/Item_8.png" },
    { 9, "assets/sprites/items/Item_9.png" },
    { 26, "assets/sprites/items/Item_26.png" },
    { 30, "assets/sprites/items/Item_30.png" },
    { 62, "assets/sprites/items/Item_62.png" },
    { 93, "assets/sprites/items/Item_93.png" },
    { 3505, "assets/sprites/items/Item_3505.png" },
    { 3506, "assets/sprites/items/Item_3506.png" },
    { 3509, "assets/sprites/items/Item_3509.png" },
};

static const std::array FONT_ASSETS = {
    std::make_pair(FontAsset::AndyBold, "andy_bold"),
    std::make_pair(FontAsset::AndyRegular, "andy_regular"),
};

const std::pair<ShaderAsset, AssetShader> SHADER_ASSETS[] = {
    { ShaderAsset::BackgroundShader,     AssetShader("background",   ShaderStages::Vertex | ShaderStages::Fragment, { VertexFormatAsset::BackgroundVertex, VertexFormatAsset::BackgroundInstance }) },
    { ShaderAsset::PostProcessShader,    AssetShader("postprocess",  ShaderStages::Vertex | ShaderStages::Fragment, VertexFormatAsset::PostProcessVertex) },
    { ShaderAsset::TilemapShader,        AssetShader("tilemap",      ShaderStages::Vertex | ShaderStages::Fragment, { VertexFormatAsset::TilemapVertex,   VertexFormatAsset::TilemapInstance   }) },
    { ShaderAsset::FontShader,           AssetShader("font",         ShaderStages::Vertex | ShaderStages::Fragment, { VertexFormatAsset::FontVertex,      VertexFormatAsset::FontInstance      }) },
    { ShaderAsset::UiFontShader,         AssetShader("ui_font",      ShaderStages::Vertex | ShaderStages::Fragment, { VertexFormatAsset::FontVertex,      VertexFormatAsset::FontInstance      }) },
    { ShaderAsset::SpriteShader,         AssetShader("sprite",       ShaderStages::Vertex | ShaderStages::Fragment, { VertexFormatAsset::SpriteVertex,    VertexFormatAsset::SpriteInstance    }) },
    { ShaderAsset::UiSpriteShader,       AssetShader("ui_sprite",    ShaderStages::Vertex | ShaderStages::Fragment, { VertexFormatAsset::SpriteVertex,    VertexFormatAsset::SpriteInstance    }) },
    { ShaderAsset::ParticleShader,       AssetShader("particle",     ShaderStages::Vertex | ShaderStages::Fragment, { VertexFormatAsset::ParticleVertex,  VertexFormatAsset::ParticleInstance  }) },
    { ShaderAsset::NinePatchShader,      AssetShader("ninepatch",    ShaderStages::Vertex | ShaderStages::Fragment, { VertexFormatAsset::NinePatchVertex, VertexFormatAsset::NinePatchInstance }) },
    { ShaderAsset::UiNinePatchShader,    AssetShader("ui_ninepatch", ShaderStages::Vertex | ShaderStages::Fragment, { VertexFormatAsset::NinePatchVertex, VertexFormatAsset::NinePatchInstance }) },
    { ShaderAsset::StaticLightMapShader, AssetShader("lightmap",     ShaderStages::Vertex | ShaderStages::Fragment, VertexFormatAsset::StaticLightMapVertex ) },
};

const std::pair<ComputeShaderAsset, AssetComputeShader> COMPUTE_SHADER_ASSETS[] = {
    { ComputeShaderAsset::ParticleComputeTransformShader, AssetComputeShader("particle", "CSComputeTransform", "particle") },
    { ComputeShaderAsset::LightVertical, AssetComputeShader("light", "CSComputeLightVertical", "light_blur_vertical") },
    { ComputeShaderAsset::LightHorizontal, AssetComputeShader("light", "CSComputeLightHorizontal", "light_blur_horizontal") },
    { ComputeShaderAsset::LightSetLightSources, AssetComputeShader("light", "CSComputeLightSetLightSources", "light_init") },
};

static struct AssetsState {
    std::unordered_map<uint16_t, Texture> items;
    std::unordered_map<TextureAsset, Texture> textures;
    std::unordered_map<TextureAsset, TextureAtlas> textures_atlases;
    std::unordered_map<ShaderAsset, ShaderPipeline> shaders;
    std::unordered_map<ComputeShaderAsset, LLGL::Shader*> compute_shaders;
    std::unordered_map<FontAsset, Font> fonts;
    std::unordered_map<VertexFormatAsset, LLGL::VertexFormat> vertex_formats;
    std::vector<Sampler> samplers;
} state;

static bool load_font(Font& font, const char* meta_file_path, const char* atlas_file_path);
static bool load_texture(const char* path, int sampler, Texture* texture);

template <size_t T>
static Texture load_texture_array(const std::array<std::tuple<uint16_t, TextureAsset, const char*>, T>& assets, int sampler, bool generate_mip_maps = false);

bool Assets::Load() {
    Renderer& renderer = Engine::Renderer();

    const uint8_t data[] = { 0xFF, 0xFF, 0xFF, 0xFF };
    state.textures[TextureAsset::Stub] = renderer.CreateTexture(LLGL::TextureType::Texture2D, LLGL::ImageFormat::RGBA, 1, 1, 1, TextureSampler::Nearest, data);

    for (const auto& [key, asset] : TEXTURE_ASSETS) {
        Texture texture;
        if (!load_texture(asset.path.c_str(), asset.sampler, &texture)) {
            return false;
        }
        state.textures[key] = texture;
    }

    for (const auto& [block_type, asset_key, path] : BLOCK_ASSETS) {
        Texture texture;
        if (!load_texture(path, TextureSampler::Nearest, &texture)) {
            return false;
        }
        state.textures[asset_key] = texture;
        state.textures_atlases[asset_key] = TextureAtlas::from_grid(texture, glm::uvec2(16, 16), (texture.width() - (texture.width() / 16) * 2) / 16 + 1, (texture.height() - (texture.height() / 16) * 2) / 16 + 1, glm::uvec2(2));
    }

    std::vector<math::Rect> background_rects(static_cast<size_t>(BackgroundAsset::Count));
    for (const auto& [id, asset_key, path] : BACKGROUND_ASSETS) {
        int width;
        int height;

        stbi_info(path, &width, &height, nullptr);

        background_rects[id] = math::URect::from_top_left(glm::uvec2(0), glm::uvec2(width, height));
    }
    state.textures_atlases[TextureAsset::Backgrounds] = TextureAtlas(Assets::GetTexture(TextureAsset::Stub), std::move(background_rects), glm::vec2(0.0f), 0, 0);

    for (const auto& [key, asset] : TEXTURE_ATLAS_ASSETS) {
        state.textures_atlases[key] = TextureAtlas::from_grid(Assets::GetTexture(key), asset.tile_size, asset.columns, asset.rows, asset.padding, asset.offset);
    }
    
    for (const auto& [key, asset] : ITEM_ASSETS) {
        Texture texture;
        if (!load_texture(asset.c_str(), TextureSampler::Nearest, &texture)) {
            return false;
        }
        state.items[key] = texture;
    }

    // There is some glitches in mipmaps on Metal
    const bool mip_maps = !renderer.Backend().IsMetal();

    state.textures[TextureAsset::Tiles] = load_texture_array(BLOCK_ASSETS, mip_maps ? TextureSampler::NearestMips : TextureSampler::Nearest, mip_maps);
    state.textures[TextureAsset::Walls] = load_texture_array(WALL_ASSETS, TextureSampler::Nearest);
    state.textures[TextureAsset::Backgrounds] = load_texture_array(BACKGROUND_ASSETS, TextureSampler::Nearest);

    return true;
}

bool Assets::LoadShaders(const std::vector<ShaderDef>& shader_defs) {
    InitVertexFormats();
    
    Renderer& renderer = Engine::Renderer();

    RenderBackend backend = renderer.Backend();

    for (const auto& [key, asset] : SHADER_ASSETS) {
        ShaderPipeline shader_pipeline;

        std::vector<LLGL::VertexAttribute> attributes;

        for (const VertexFormatAsset asset : asset.vertex_format_assets) {
            const LLGL::VertexFormat& vertex_format = Assets::GetVertexFormat(asset);
            attributes.insert(attributes.end(), vertex_format.attributes.begin(), vertex_format.attributes.end());
        }

        if (check_bitflag(asset.stages, ShaderStages::Vertex)) {
            if (!(shader_pipeline.vs = renderer.LoadShader(ShaderPath(ShaderType::Vertex, asset.file_name), shader_defs, attributes)))
                return false;
        }
        
        if (check_bitflag(asset.stages, ShaderStages::Fragment)) {
            if (!(shader_pipeline.ps = renderer.LoadShader(ShaderPath(ShaderType::Fragment, asset.file_name), shader_defs)))
                return false;
        }

        if (check_bitflag(asset.stages, ShaderStages::Geometry)) {
            if (!(shader_pipeline.gs = renderer.LoadShader(ShaderPath(ShaderType::Geometry, asset.file_name), shader_defs)))
                return false;
        }

        state.shaders[key] = shader_pipeline;
    }

    for (const auto& [key, asset] : COMPUTE_SHADER_ASSETS) {
        const std::string& file_name = backend.IsGLSL() ? asset.glsl_file_name : asset.file_name;

        if (!(state.compute_shaders[key] = renderer.LoadShader(ShaderPath(ShaderType::Compute, file_name, asset.func_name), shader_defs)))
            return false;
    }

    return true;
};

bool Assets::LoadFonts() {
    for (const auto& [key, name] : FONT_ASSETS) {
        const fs::path fonts_folder = fs::path("assets") / "fonts";
        const fs::path meta_file = fonts_folder / fs::path(name).concat(".meta");
        const fs::path atlas_file = fonts_folder / fs::path(name).concat(".png");

        const std::string meta_file_str = meta_file.string();
        const std::string atlas_file_str = atlas_file.string();

        if (!FileExists(meta_file_str.c_str())) {
            LOG_ERROR("Failed to find the font meta file '%s'", meta_file_str.c_str());
            return false;
        }

        if (!FileExists(atlas_file_str.c_str())) {
            LOG_ERROR("Failed to find the font atlas file '%s'", atlas_file_str.c_str());
            return false;
        }

        Font font;
        if (!load_font(font, meta_file_str.c_str(), atlas_file_str.c_str())) return false;

        state.fonts[key] = font;
    }

    return true;
}

bool Assets::InitSamplers() {
    Renderer& renderer = Engine::Renderer();
    const auto& context = renderer.Context();

    state.samplers.resize(4);
    {
        LLGL::SamplerDescriptor sampler_desc;
        sampler_desc.addressModeU = LLGL::SamplerAddressMode::Clamp;
        sampler_desc.addressModeV = LLGL::SamplerAddressMode::Clamp;
        sampler_desc.addressModeW = LLGL::SamplerAddressMode::Clamp;
        sampler_desc.magFilter = LLGL::SamplerFilter::Linear;
        sampler_desc.minFilter = LLGL::SamplerFilter::Linear;
        sampler_desc.mipMapFilter = LLGL::SamplerFilter::Linear;
        sampler_desc.minLOD = 0.0f;
        sampler_desc.maxLOD = 0.0f;
        sampler_desc.mipMapEnabled = false;
        sampler_desc.maxAnisotropy = 1;

        state.samplers[TextureSampler::Linear] = Sampler(context->CreateSampler(sampler_desc), sampler_desc);
    }
    {
        LLGL::SamplerDescriptor sampler_desc;
        sampler_desc.addressModeU = LLGL::SamplerAddressMode::Clamp;
        sampler_desc.addressModeV = LLGL::SamplerAddressMode::Clamp;
        sampler_desc.addressModeW = LLGL::SamplerAddressMode::Clamp;
        sampler_desc.magFilter = LLGL::SamplerFilter::Linear;
        sampler_desc.minFilter = LLGL::SamplerFilter::Linear;
        sampler_desc.mipMapFilter = LLGL::SamplerFilter::Linear;
        sampler_desc.minLOD = 0.0f;
        sampler_desc.maxLOD = 100.0f;
        sampler_desc.mipMapEnabled = true;
        sampler_desc.maxAnisotropy = 1;

        state.samplers[TextureSampler::LinearMips] = Sampler(context->CreateSampler(sampler_desc), sampler_desc);
    }
    {
        LLGL::SamplerDescriptor sampler_desc;
        sampler_desc.addressModeU = LLGL::SamplerAddressMode::Clamp;
        sampler_desc.addressModeV = LLGL::SamplerAddressMode::Clamp;
        sampler_desc.addressModeW = LLGL::SamplerAddressMode::Clamp;
        sampler_desc.magFilter = LLGL::SamplerFilter::Nearest;
        sampler_desc.minFilter = LLGL::SamplerFilter::Nearest;
        sampler_desc.mipMapFilter = LLGL::SamplerFilter::Linear;
        sampler_desc.minLOD = 0.0f;
        sampler_desc.maxLOD = 0.0f;
        sampler_desc.mipMapEnabled = false;
        sampler_desc.maxAnisotropy = 1;

        state.samplers[TextureSampler::Nearest] = Sampler(context->CreateSampler(sampler_desc), sampler_desc);
    }
    {
        LLGL::SamplerDescriptor sampler_desc;
        sampler_desc.addressModeU = LLGL::SamplerAddressMode::Clamp;
        sampler_desc.addressModeV = LLGL::SamplerAddressMode::Clamp;
        sampler_desc.addressModeW = LLGL::SamplerAddressMode::Clamp;
        sampler_desc.magFilter = LLGL::SamplerFilter::Nearest;
        sampler_desc.minFilter = LLGL::SamplerFilter::Nearest;
        sampler_desc.mipMapFilter = LLGL::SamplerFilter::Linear;
        sampler_desc.minLOD = 0.0f;
        sampler_desc.maxLOD = 100.0f;
        sampler_desc.mipMapEnabled = true;
        sampler_desc.maxAnisotropy = 1;

        state.samplers[TextureSampler::NearestMips] = Sampler(context->CreateSampler(sampler_desc), sampler_desc);
    }

    return true;
}

void Assets::InitVertexFormats() {
    Renderer& renderer = Engine::Renderer();

    const RenderBackend backend = renderer.Backend();

    LLGL::VertexFormat sprite_vertex_format;
    LLGL::VertexFormat sprite_instance_format;
    LLGL::VertexFormat ninepatch_vertex_format;
    LLGL::VertexFormat ninepatch_instance_format;
    LLGL::VertexFormat tilemap_vertex_format;
    LLGL::VertexFormat tilemap_instance_format;
    LLGL::VertexFormat font_vertex_format;
    LLGL::VertexFormat font_instance_format;
    LLGL::VertexFormat background_vertex_format;
    LLGL::VertexFormat background_instance_format;
    LLGL::VertexFormat particle_vertex_format;
    LLGL::VertexFormat particle_instance_format;
    LLGL::VertexFormat postprocess_vertex_format;
    LLGL::VertexFormat static_lightmap_vertex_format;

    if (backend.IsGLSL()) {
        sprite_vertex_format.AppendAttribute({ "a_position", LLGL::Format::RG32Float, 0, 0, sizeof(Vertex), 0, 0 });
    } else if (backend.IsHLSL()) {
        sprite_vertex_format.AppendAttribute({ "Position",   LLGL::Format::RG32Float, 0, 0, sizeof(Vertex), 0, 0 });
    } else {
        sprite_vertex_format.AppendAttribute({ "position",   LLGL::Format::RG32Float, 0, 0, sizeof(Vertex), 0, 0 });
    }

    if (backend.IsGLSL()) {
        sprite_instance_format.attributes = {
            {"i_position",           LLGL::Format::RGB32Float,  1,  offsetof(SpriteInstance,position),          sizeof(SpriteInstance), 1, 1 },
            {"i_rotation",           LLGL::Format::RGBA32Float, 2,  offsetof(SpriteInstance,rotation),          sizeof(SpriteInstance), 1, 1 },
            {"i_size",               LLGL::Format::RG32Float,   3,  offsetof(SpriteInstance,size),              sizeof(SpriteInstance), 1, 1 },
            {"i_offset",             LLGL::Format::RG32Float,   4,  offsetof(SpriteInstance,offset),            sizeof(SpriteInstance), 1, 1 },
            {"i_uv_offset_scale",    LLGL::Format::RGBA32Float, 5,  offsetof(SpriteInstance,uv_offset_scale),   sizeof(SpriteInstance), 1, 1 },
            {"i_color",              LLGL::Format::RGBA32Float, 6,  offsetof(SpriteInstance,color),             sizeof(SpriteInstance), 1, 1 },
            {"i_outline_color",      LLGL::Format::RGBA32Float, 7,  offsetof(SpriteInstance,outline_color),     sizeof(SpriteInstance), 1, 1 },
            {"i_outline_thickness",  LLGL::Format::R32Float,    8,  offsetof(SpriteInstance,outline_thickness), sizeof(SpriteInstance), 1, 1 },
            {"i_flags",              LLGL::Format::R32SInt,     9,  offsetof(SpriteInstance,flags),             sizeof(SpriteInstance), 1, 1 },
        };
    } else if (backend.IsHLSL()) {
        sprite_instance_format.attributes = {
            {"I_Position",         LLGL::Format::RGB32Float,  1,  offsetof(SpriteInstance,position),          sizeof(SpriteInstance), 1, 1 },
            {"I_Rotation",         LLGL::Format::RGBA32Float, 2,  offsetof(SpriteInstance,rotation),          sizeof(SpriteInstance), 1, 1 },
            {"I_Size",             LLGL::Format::RG32Float,   3,  offsetof(SpriteInstance,size),              sizeof(SpriteInstance), 1, 1 },
            {"I_Offset",           LLGL::Format::RG32Float,   4,  offsetof(SpriteInstance,offset),            sizeof(SpriteInstance), 1, 1 },
            {"I_UvOffsetScale",    LLGL::Format::RGBA32Float, 5,  offsetof(SpriteInstance,uv_offset_scale),   sizeof(SpriteInstance), 1, 1 },
            {"I_Color",            LLGL::Format::RGBA32Float, 6,  offsetof(SpriteInstance,color),             sizeof(SpriteInstance), 1, 1 },
            {"I_OutlineColor",     LLGL::Format::RGBA32Float, 7,  offsetof(SpriteInstance,outline_color),     sizeof(SpriteInstance), 1, 1 },
            {"I_OutlineThickness", LLGL::Format::R32Float,    8,  offsetof(SpriteInstance,outline_thickness), sizeof(SpriteInstance), 1, 1 },
            {"I_Flags",            LLGL::Format::R32SInt,     9,  offsetof(SpriteInstance,flags),             sizeof(SpriteInstance), 1, 1 },
        };
    } else {
        sprite_instance_format.attributes = {
            {"i_position",           LLGL::Format::RGB32Float,  1,  offsetof(SpriteInstance,position),          sizeof(SpriteInstance), 1, 1 },
            {"i_rotation",           LLGL::Format::RGBA32Float, 2,  offsetof(SpriteInstance,rotation),          sizeof(SpriteInstance), 1, 1 },
            {"i_size",               LLGL::Format::RG32Float,   3,  offsetof(SpriteInstance,size),              sizeof(SpriteInstance), 1, 1 },
            {"i_offset",             LLGL::Format::RG32Float,   4,  offsetof(SpriteInstance,offset),            sizeof(SpriteInstance), 1, 1 },
            {"i_uv_offset_scale",    LLGL::Format::RGBA32Float, 5,  offsetof(SpriteInstance,uv_offset_scale),   sizeof(SpriteInstance), 1, 1 },
            {"i_color",              LLGL::Format::RGBA32Float, 6,  offsetof(SpriteInstance,color),             sizeof(SpriteInstance), 1, 1 },
            {"i_outline_color",      LLGL::Format::RGBA32Float, 7,  offsetof(SpriteInstance,outline_color),     sizeof(SpriteInstance), 1, 1 },
            {"i_outline_thickness",  LLGL::Format::R32Float,    8,  offsetof(SpriteInstance,outline_thickness), sizeof(SpriteInstance), 1, 1 },
            {"i_flags",              LLGL::Format::R32SInt,     9,  offsetof(SpriteInstance,flags),             sizeof(SpriteInstance), 1, 1 },
        };
    }

    if (backend.IsGLSL()) {
        ninepatch_vertex_format.AppendAttribute({ "a_position", LLGL::Format::RG32Float, 0, 0, sizeof(Vertex), 0, 0 });
    } else if (backend.IsHLSL()) {
        ninepatch_vertex_format.AppendAttribute({ "Position",   LLGL::Format::RG32Float, 0, 0, sizeof(Vertex), 0, 0 });
    } else {
        ninepatch_vertex_format.AppendAttribute({ "position",   LLGL::Format::RG32Float, 0, 0, sizeof(Vertex), 0, 0 });
    }

    if (backend.IsGLSL()) {
        ninepatch_instance_format.attributes = {
            {"i_position",         LLGL::Format::RG32Float,   1,  offsetof(NinePatchInstance,position),          sizeof(NinePatchInstance), 1, 1 },
            {"i_rotation",         LLGL::Format::RGBA32Float, 2,  offsetof(NinePatchInstance,rotation),          sizeof(NinePatchInstance), 1, 1 },
            {"i_size",             LLGL::Format::RG32Float,   3,  offsetof(NinePatchInstance,size),              sizeof(NinePatchInstance), 1, 1 },
            {"i_offset",           LLGL::Format::RG32Float,   4,  offsetof(NinePatchInstance,offset),            sizeof(NinePatchInstance), 1, 1 },
            {"i_source_size",      LLGL::Format::RG32Float,   5,  offsetof(NinePatchInstance,source_size),       sizeof(NinePatchInstance), 1, 1 },
            {"i_output_size",      LLGL::Format::RG32Float,   6,  offsetof(NinePatchInstance,output_size),       sizeof(NinePatchInstance), 1, 1 },
            {"i_margin",           LLGL::Format::RGBA32UInt,  7,  offsetof(NinePatchInstance,margin),            sizeof(NinePatchInstance), 1, 1 },
            {"i_uv_offset_scale",  LLGL::Format::RGBA32Float, 8,  offsetof(NinePatchInstance,uv_offset_scale),   sizeof(NinePatchInstance), 1, 1 },
            {"i_color",            LLGL::Format::RGBA32Float, 9,  offsetof(NinePatchInstance,color),             sizeof(NinePatchInstance), 1, 1 },
            {"i_flags",            LLGL::Format::R32SInt,     10, offsetof(NinePatchInstance,flags),             sizeof(NinePatchInstance), 1, 1 },
        };
    } else if (backend.IsHLSL()) {
        ninepatch_instance_format.attributes = {
            {"I_Position",         LLGL::Format::RG32Float,   1,  offsetof(NinePatchInstance,position),          sizeof(NinePatchInstance), 1, 1 },
            {"I_Rotation",         LLGL::Format::RGBA32Float, 2,  offsetof(NinePatchInstance,rotation),          sizeof(NinePatchInstance), 1, 1 },
            {"I_Size",             LLGL::Format::RG32Float,   3,  offsetof(NinePatchInstance,size),              sizeof(NinePatchInstance), 1, 1 },
            {"I_Offset",           LLGL::Format::RG32Float,   4,  offsetof(NinePatchInstance,offset),            sizeof(NinePatchInstance), 1, 1 },
            {"I_SourceSize",       LLGL::Format::RG32Float,   5,  offsetof(NinePatchInstance,source_size),       sizeof(NinePatchInstance), 1, 1 },
            {"I_OutputSize",       LLGL::Format::RG32Float,   6,  offsetof(NinePatchInstance,output_size),       sizeof(NinePatchInstance), 1, 1 },
            {"I_Margin",           LLGL::Format::RGBA32UInt,  7,  offsetof(NinePatchInstance,margin),            sizeof(NinePatchInstance), 1, 1 },
            {"I_UvOffsetScale",    LLGL::Format::RGBA32Float, 8,  offsetof(NinePatchInstance,uv_offset_scale),   sizeof(NinePatchInstance), 1, 1 },
            {"I_Color",            LLGL::Format::RGBA32Float, 9,  offsetof(NinePatchInstance,color),             sizeof(NinePatchInstance), 1, 1 },
            {"I_Flags",            LLGL::Format::R32SInt,     10, offsetof(NinePatchInstance,flags),             sizeof(NinePatchInstance), 1, 1 },
        };
    } else {
        ninepatch_instance_format.attributes = {
            {"i_position",         LLGL::Format::RG32Float,   1,  offsetof(NinePatchInstance,position),          sizeof(NinePatchInstance), 1, 1 },
            {"i_rotation",         LLGL::Format::RGBA32Float, 2,  offsetof(NinePatchInstance,rotation),          sizeof(NinePatchInstance), 1, 1 },
            {"i_size",             LLGL::Format::RG32Float,   3,  offsetof(NinePatchInstance,size),              sizeof(NinePatchInstance), 1, 1 },
            {"i_offset",           LLGL::Format::RG32Float,   4,  offsetof(NinePatchInstance,offset),            sizeof(NinePatchInstance), 1, 1 },
            {"i_source_size",      LLGL::Format::RG32Float,   5,  offsetof(NinePatchInstance,source_size),       sizeof(NinePatchInstance), 1, 1 },
            {"i_output_size",      LLGL::Format::RG32Float,   6,  offsetof(NinePatchInstance,output_size),       sizeof(NinePatchInstance), 1, 1 },
            {"i_margin",           LLGL::Format::RGBA32UInt,  7,  offsetof(NinePatchInstance,margin),            sizeof(NinePatchInstance), 1, 1 },
            {"i_uv_offset_scale",  LLGL::Format::RGBA32Float, 8,  offsetof(NinePatchInstance,uv_offset_scale),   sizeof(NinePatchInstance), 1, 1 },
            {"i_color",            LLGL::Format::RGBA32Float, 9,  offsetof(NinePatchInstance,color),             sizeof(NinePatchInstance), 1, 1 },
            {"i_flags",            LLGL::Format::R32SInt,     10, offsetof(NinePatchInstance,flags),             sizeof(NinePatchInstance), 1, 1 },
        };
    }

    if (backend.IsGLSL()) {
        tilemap_vertex_format.attributes = {
            {"a_position", LLGL::Format::RG32Float, 0, 0, sizeof(ChunkVertex), 0, 0 },
            {"a_wall_tex_size", LLGL::Format::RG32Float, 1, offsetof(ChunkVertex,wall_tex_size), sizeof(ChunkVertex), 0, 0},
            {"a_tile_tex_size", LLGL::Format::RG32Float, 2, offsetof(ChunkVertex,tile_tex_size), sizeof(ChunkVertex), 0, 0},
            {"a_wall_padding",  LLGL::Format::RG32Float, 3, offsetof(ChunkVertex,wall_padding),  sizeof(ChunkVertex), 0, 0},
            {"a_tile_padding",  LLGL::Format::RG32Float, 4, offsetof(ChunkVertex,tile_padding),  sizeof(ChunkVertex), 0, 0}
        };
    } else if (backend.IsHLSL()) {
        tilemap_vertex_format.attributes = {
            {"Position"   , LLGL::Format::RG32Float, 0, 0, sizeof(ChunkVertex), 0, 0 },
            {"WallTexSize", LLGL::Format::RG32Float, 1, offsetof(ChunkVertex,wall_tex_size), sizeof(ChunkVertex), 0, 0},
            {"TileTexSize", LLGL::Format::RG32Float, 2, offsetof(ChunkVertex,tile_tex_size), sizeof(ChunkVertex), 0, 0},
            {"WallPadding", LLGL::Format::RG32Float, 3, offsetof(ChunkVertex,wall_padding), sizeof(ChunkVertex), 0, 0},
            {"TilePadding", LLGL::Format::RG32Float, 4, offsetof(ChunkVertex,tile_padding), sizeof(ChunkVertex), 0, 0},
        };
    } else {
        tilemap_vertex_format.attributes = {
            {"position",      LLGL::Format::RG32Float, 0, 0, sizeof(ChunkVertex), 0, 0 },
            {"wall_tex_size", LLGL::Format::RG32Float, 1, offsetof(ChunkVertex,wall_tex_size), sizeof(ChunkVertex), 0, 0},
            {"tile_tex_size", LLGL::Format::RG32Float, 2, offsetof(ChunkVertex,tile_tex_size), sizeof(ChunkVertex), 0, 0},
            {"wall_padding",  LLGL::Format::RG32Float, 3, offsetof(ChunkVertex,wall_padding),  sizeof(ChunkVertex), 0, 0},
            {"tile_padding",  LLGL::Format::RG32Float, 4, offsetof(ChunkVertex,tile_padding),  sizeof(ChunkVertex), 0, 0}
        };
    }

    if (backend.IsGLSL()) {
        tilemap_instance_format.attributes = {
            {"i_position",  LLGL::Format::RG32Float,  5, offsetof(ChunkInstance,position),  sizeof(ChunkInstance), 1, 1},
            {"i_atlas_pos", LLGL::Format::RG32Float,  6, offsetof(ChunkInstance,atlas_pos), sizeof(ChunkInstance), 1, 1},
            {"i_world_pos", LLGL::Format::RG32Float,  7, offsetof(ChunkInstance,world_pos), sizeof(ChunkInstance), 1, 1},
            {"i_tile_data", LLGL::Format::R32UInt,    8, offsetof(ChunkInstance,tile_data), sizeof(ChunkInstance), 1, 1},
        };
    } else if (backend.IsHLSL()) {
        tilemap_instance_format.attributes = {
            {"I_Position",  LLGL::Format::RG32Float,  5, offsetof(ChunkInstance,position),  sizeof(ChunkInstance), 1, 1},
            {"I_AtlasPos",  LLGL::Format::RG32Float,  6, offsetof(ChunkInstance,atlas_pos), sizeof(ChunkInstance), 1, 1},
            {"I_WorldPos",  LLGL::Format::RG32Float,  7, offsetof(ChunkInstance,world_pos), sizeof(ChunkInstance), 1, 1},
            {"I_TileData",  LLGL::Format::R16UInt,    8, offsetof(ChunkInstance,tile_data), sizeof(ChunkInstance), 1, 1},
        };
    } else {
        tilemap_instance_format.attributes = {
            {"i_position",  LLGL::Format::RG32Float,  5, offsetof(ChunkInstance,position),  sizeof(ChunkInstance), 1, 1},
            {"i_atlas_pos", LLGL::Format::RG32Float,  6, offsetof(ChunkInstance,atlas_pos), sizeof(ChunkInstance), 1, 1},
            {"i_world_pos", LLGL::Format::RG32Float,  7, offsetof(ChunkInstance,world_pos), sizeof(ChunkInstance), 1, 1},
            {"i_tile_data", LLGL::Format::R32UInt,    8, offsetof(ChunkInstance,tile_data), sizeof(ChunkInstance), 1, 1},
        };
    }

    if (backend.IsGLSL()) {
        font_vertex_format.AppendAttribute({"a_position", LLGL::Format::RG32Float, 0, 0, sizeof(Vertex), 0, 0});
    } else if (backend.IsHLSL()) {
        font_vertex_format.AppendAttribute({"Position",   LLGL::Format::RG32Float, 0, 0, sizeof(Vertex), 0, 0});
    } else {
        font_vertex_format.AppendAttribute({"position",   LLGL::Format::RG32Float, 0, 0, sizeof(Vertex), 0, 0});
    }

    if (backend.IsGLSL()) {
        font_instance_format.attributes = {
            {"i_color",     LLGL::Format::RGB32Float, 1, offsetof(GlyphInstance,color),    sizeof(GlyphInstance), 1, 1},
            {"i_position",  LLGL::Format::RGB32Float, 2, offsetof(GlyphInstance,pos),      sizeof(GlyphInstance), 1, 1},
            {"i_size",      LLGL::Format::RG32Float,  3, offsetof(GlyphInstance,size),     sizeof(GlyphInstance), 1, 1},
            {"i_tex_size",  LLGL::Format::RG32Float,  4, offsetof(GlyphInstance,tex_size), sizeof(GlyphInstance), 1, 1},
            {"i_uv",        LLGL::Format::RG32Float,  5, offsetof(GlyphInstance,uv),       sizeof(GlyphInstance), 1, 1},
        };
    } else if (backend.IsHLSL()) {
        font_instance_format.attributes = {
            {"I_Color",    LLGL::Format::RGB32Float, 1, offsetof(GlyphInstance,color),    sizeof(GlyphInstance), 1, 1},
            {"I_Position", LLGL::Format::RG32Float,  2, offsetof(GlyphInstance,pos),      sizeof(GlyphInstance), 1, 1},
            {"I_Size",     LLGL::Format::RG32Float,  3, offsetof(GlyphInstance,size),     sizeof(GlyphInstance), 1, 1},
            {"I_TexSize",  LLGL::Format::RG32Float,  4, offsetof(GlyphInstance,tex_size), sizeof(GlyphInstance), 1, 1},
            {"I_UV",       LLGL::Format::RG32Float,  5, offsetof(GlyphInstance,uv),       sizeof(GlyphInstance), 1, 1},
        };
    } else {
        font_instance_format.attributes = {
            {"i_color",     LLGL::Format::RGB32Float, 1, offsetof(GlyphInstance,color),    sizeof(GlyphInstance), 1, 1},
            {"i_position",  LLGL::Format::RGB32Float, 2, offsetof(GlyphInstance,pos),      sizeof(GlyphInstance), 1, 1},
            {"i_size",      LLGL::Format::RG32Float,  3, offsetof(GlyphInstance,size),     sizeof(GlyphInstance), 1, 1},
            {"i_tex_size",  LLGL::Format::RG32Float,  4, offsetof(GlyphInstance,tex_size), sizeof(GlyphInstance), 1, 1},
            {"i_uv",        LLGL::Format::RG32Float,  5, offsetof(GlyphInstance,uv),       sizeof(GlyphInstance), 1, 1},
        };
    }

    if (backend.IsGLSL()) {
        background_vertex_format.attributes = {
            {"a_position",     LLGL::Format::RG32Float, 0, 0, sizeof(BackgroundVertex), 0, 0},
            {"a_texture_size", LLGL::Format::RG32Float, 1, offsetof(BackgroundVertex,texture_size), sizeof(BackgroundVertex), 0, 0},
        };
    } else if (backend.IsHLSL()) {
        background_vertex_format.attributes = {
            {"Position",       LLGL::Format::RG32Float, 0, 0, sizeof(BackgroundVertex), 0, 0},
            {"TextureSize",    LLGL::Format::RG32Float, 1, offsetof(BackgroundVertex,texture_size), sizeof(BackgroundVertex), 0, 0},
        };
    } else {
        background_vertex_format.attributes = {
            {"position",       LLGL::Format::RG32Float, 0, 0, sizeof(BackgroundVertex), 0, 0},
            {"a_texture_size", LLGL::Format::RG32Float, 1, offsetof(BackgroundVertex,texture_size), sizeof(BackgroundVertex), 0, 0},
        };
    }

    if (backend.IsGLSL()) {
        background_instance_format.attributes = {
            {"i_position", LLGL::Format::RG32Float,  2, offsetof(BackgroundInstance, position), sizeof(BackgroundInstance), 1, 1},
            {"i_size",     LLGL::Format::RG32Float,  3, offsetof(BackgroundInstance, size),     sizeof(BackgroundInstance), 1, 1},
            {"i_tex_size", LLGL::Format::RG32Float,  4, offsetof(BackgroundInstance, tex_size), sizeof(BackgroundInstance), 1, 1},
            {"i_speed",    LLGL::Format::RG32Float,  5, offsetof(BackgroundInstance, speed),    sizeof(BackgroundInstance), 1, 1},
            {"i_id",       LLGL::Format::R32UInt,    6, offsetof(BackgroundInstance, id),       sizeof(BackgroundInstance), 1, 1},
            {"i_flags",    LLGL::Format::R32SInt,    7, offsetof(BackgroundInstance, flags),    sizeof(BackgroundInstance), 1, 1},
        };
    } else if (backend.IsHLSL()) {
        background_instance_format.attributes = {
            {"I_Position", LLGL::Format::RG32Float,  2, offsetof(BackgroundInstance, position), sizeof(BackgroundInstance), 1, 1},
            {"I_Size",     LLGL::Format::RG32Float,  3, offsetof(BackgroundInstance, size),     sizeof(BackgroundInstance), 1, 1},
            {"I_TexSize",  LLGL::Format::RG32Float,  4, offsetof(BackgroundInstance, tex_size), sizeof(BackgroundInstance), 1, 1},
            {"I_Speed",    LLGL::Format::RG32Float,  5, offsetof(BackgroundInstance, speed),    sizeof(BackgroundInstance), 1, 1},
            {"I_ID",       LLGL::Format::R32UInt,    6, offsetof(BackgroundInstance, id),       sizeof(BackgroundInstance), 1, 1},
            {"I_Flags",    LLGL::Format::R32SInt,    7, offsetof(BackgroundInstance, flags),    sizeof(BackgroundInstance), 1, 1},
        };
    } else {
        background_instance_format.attributes = {
            {"i_position", LLGL::Format::RG32Float,  2, offsetof(BackgroundInstance, position), sizeof(BackgroundInstance), 1, 1},
            {"i_size",     LLGL::Format::RG32Float,  3, offsetof(BackgroundInstance, size),     sizeof(BackgroundInstance), 1, 1},
            {"i_tex_size", LLGL::Format::RG32Float,  4, offsetof(BackgroundInstance, tex_size), sizeof(BackgroundInstance), 1, 1},
            {"i_speed",    LLGL::Format::RG32Float,  5, offsetof(BackgroundInstance, speed),    sizeof(BackgroundInstance), 1, 1},
            {"i_id",       LLGL::Format::R32UInt,    6, offsetof(BackgroundInstance, id),       sizeof(BackgroundInstance), 1, 1},
            {"i_flags",    LLGL::Format::R32SInt,    7, offsetof(BackgroundInstance, flags),    sizeof(BackgroundInstance), 1, 1},
        };
    }

    if (backend.IsGLSL()) {
        particle_vertex_format.attributes = {
            { "a_position",      LLGL::Format::RG32Float, 0, 0,                                      sizeof(ParticleVertex), 0 },
            { "a_inv_tex_size",  LLGL::Format::RG32Float, 1, offsetof(ParticleVertex, inv_tex_size), sizeof(ParticleVertex), 0 },
            { "a_tex_size",      LLGL::Format::RG32Float, 2, offsetof(ParticleVertex, tex_size),     sizeof(ParticleVertex), 0 }
        };
    } else if (backend.IsHLSL()) {
        particle_vertex_format.attributes = {
            { "Position",   LLGL::Format::RG32Float, 0, 0,                                      sizeof(ParticleVertex), 0 },
            { "InvTexSize", LLGL::Format::RG32Float, 1, offsetof(ParticleVertex, inv_tex_size), sizeof(ParticleVertex), 0 },
            { "TexSize",    LLGL::Format::RG32Float, 2, offsetof(ParticleVertex, tex_size),     sizeof(ParticleVertex), 0 },
        };
    } else {
        particle_vertex_format.attributes = {
            { "position",     LLGL::Format::RG32Float, 0, 0,                                      sizeof(ParticleVertex), 0 },
            { "inv_tex_size", LLGL::Format::RG32Float, 1, offsetof(ParticleVertex, inv_tex_size), sizeof(ParticleVertex), 0 },
            { "tex_size",     LLGL::Format::RG32Float, 2, offsetof(ParticleVertex, tex_size),     sizeof(ParticleVertex), 0 }
        };
    }

    if (backend.IsGLSL()) {
        particle_instance_format.attributes = {
            { "i_uv",       LLGL::Format::RG32Float, 3, offsetof(ParticleInstance, uv),       sizeof(ParticleInstance), 1, 1},
            { "i_depth",    LLGL::Format::R32Float,  4, offsetof(ParticleInstance, depth),    sizeof(ParticleInstance), 1, 1},
            { "i_id",       LLGL::Format::R32UInt,   5, offsetof(ParticleInstance, id),       sizeof(ParticleInstance), 1, 1 },
            { "I_is_world", LLGL::Format::R32UInt,   6, offsetof(ParticleInstance, is_world), sizeof(ParticleInstance), 1, 1 }
        };
    } else if (backend.IsHLSL()) {
        particle_instance_format.attributes = {
            { "I_UV",      LLGL::Format::RG32Float, 3, offsetof(ParticleInstance, uv),       sizeof(ParticleInstance), 1, 1 },
            { "I_Depth",   LLGL::Format::R32Float,  4, offsetof(ParticleInstance, depth),    sizeof(ParticleInstance), 1, 1 },
            { "I_ID",      LLGL::Format::R32UInt,   5, offsetof(ParticleInstance, id),       sizeof(ParticleInstance), 1, 1 },
            { "I_IsWorld", LLGL::Format::R32UInt,   6, offsetof(ParticleInstance, is_world), sizeof(ParticleInstance), 1, 1 }
        };
    } else {
        particle_instance_format.attributes = {
            { "i_uv",       LLGL::Format::RG32Float, 3, offsetof(ParticleInstance, uv),       sizeof(ParticleInstance), 1, 1 },
            { "i_depth",    LLGL::Format::R32Float,  4, offsetof(ParticleInstance, depth),    sizeof(ParticleInstance), 1, 1 },
            { "i_id",       LLGL::Format::R32UInt,   5, offsetof(ParticleInstance, id),       sizeof(ParticleInstance), 1, 1 },
            { "I_is_world", LLGL::Format::R32UInt,   6, offsetof(ParticleInstance, is_world), sizeof(ParticleInstance), 1, 1 }
        };
    }

    if (backend.IsGLSL()) {
        postprocess_vertex_format.AppendAttribute({ "a_position",   LLGL::Format::RG32Float });
        postprocess_vertex_format.AppendAttribute({ "a_uv",         LLGL::Format::RG32Float });
        postprocess_vertex_format.AppendAttribute({ "a_world_size", LLGL::Format::RG32Float });
    } else if (backend.IsHLSL()) {
        postprocess_vertex_format.AppendAttribute({ "Position",  LLGL::Format::RG32Float });
        postprocess_vertex_format.AppendAttribute({ "UV",        LLGL::Format::RG32Float });
        postprocess_vertex_format.AppendAttribute({ "WorldSize", LLGL::Format::RG32Float });
    } else {
        postprocess_vertex_format.AppendAttribute({ "position",  LLGL::Format::RG32Float });
        postprocess_vertex_format.AppendAttribute({ "uv",        LLGL::Format::RG32Float });
        postprocess_vertex_format.AppendAttribute({ "world_size", LLGL::Format::RG32Float });
    }

    if (backend.IsGLSL()) {
        static_lightmap_vertex_format.attributes = {
            { "a_position", LLGL::Format::RG32Float, 0, offsetof(StaticLightMapChunkVertex, position), sizeof(StaticLightMapChunkVertex), 0 },
            { "a_uv",       LLGL::Format::RG32Float, 1, offsetof(StaticLightMapChunkVertex, uv),       sizeof(StaticLightMapChunkVertex), 0 },
        };
    } else if (backend.IsHLSL()) {
        static_lightmap_vertex_format.attributes = {
            { "Position", LLGL::Format::RG32Float, 0, offsetof(StaticLightMapChunkVertex, position), sizeof(StaticLightMapChunkVertex), 0 },
            { "UV",       LLGL::Format::RG32Float, 1, offsetof(StaticLightMapChunkVertex, uv),       sizeof(StaticLightMapChunkVertex), 0 },
        };
    } else {
        static_lightmap_vertex_format.attributes = {
            { "position", LLGL::Format::RG32Float, 0, offsetof(StaticLightMapChunkVertex, position), sizeof(StaticLightMapChunkVertex), 0 },
            { "uv",       LLGL::Format::RG32Float, 1, offsetof(StaticLightMapChunkVertex, uv),       sizeof(StaticLightMapChunkVertex), 0 },
        };
    }

    state.vertex_formats[VertexFormatAsset::SpriteVertex] = sprite_vertex_format;
    state.vertex_formats[VertexFormatAsset::SpriteInstance] = sprite_instance_format;
    state.vertex_formats[VertexFormatAsset::NinePatchVertex] = ninepatch_vertex_format;
    state.vertex_formats[VertexFormatAsset::NinePatchInstance] = ninepatch_instance_format;
    state.vertex_formats[VertexFormatAsset::TilemapVertex] = tilemap_vertex_format;
    state.vertex_formats[VertexFormatAsset::TilemapInstance] = tilemap_instance_format;
    state.vertex_formats[VertexFormatAsset::FontVertex] = font_vertex_format;
    state.vertex_formats[VertexFormatAsset::FontInstance] = font_instance_format;
    state.vertex_formats[VertexFormatAsset::BackgroundVertex] = background_vertex_format;
    state.vertex_formats[VertexFormatAsset::BackgroundInstance] = background_instance_format;
    state.vertex_formats[VertexFormatAsset::ParticleVertex] = particle_vertex_format;
    state.vertex_formats[VertexFormatAsset::ParticleInstance] = particle_instance_format;
    state.vertex_formats[VertexFormatAsset::PostProcessVertex] = postprocess_vertex_format;
    state.vertex_formats[VertexFormatAsset::StaticLightMapVertex] = static_lightmap_vertex_format;
}

void Assets::DestroyTextures() {
    const auto& context = Engine::Renderer().Context();

    for (auto& entry : state.textures) {
        context->Release(entry.second);
    }
}

void Assets::DestroySamplers() {
    const auto& context = Engine::Renderer().Context();

    for (auto& sampler : state.samplers) {
        context->Release(sampler);
    }
}

void Assets::DestroyShaders() {
    const auto& context = Engine::Renderer().Context();

    for (auto& entry : state.shaders) {
        entry.second.Unload(context);
    }
}

const Texture& Assets::GetTexture(TextureAsset key) {
    const auto entry = std::as_const(state.textures).find(key);
    ASSERT(entry != state.textures.cend(), "Texture not found: %u", static_cast<uint32_t>(key));
    return entry->second;
}

const TextureAtlas& Assets::GetTextureAtlas(TextureAsset key) {
    const auto entry = std::as_const(state.textures_atlases).find(key);
    ASSERT(entry != state.textures_atlases.cend(), "TextureAtlas not found: %u", static_cast<uint32_t>(key));
    return entry->second;
}

const Font& Assets::GetFont(FontAsset key) {
    const auto entry = std::as_const(state.fonts).find(key);
    ASSERT(entry != state.fonts.cend(), "Font not found: %u", static_cast<uint32_t>(key));
    return entry->second;
}

const Texture& Assets::GetItemTexture(uint16_t index) {
    const auto entry = std::as_const(state.items).find(index);
    ASSERT(entry != state.items.cend(), "Item not found: %u", index);
    return entry->second;
}

const ShaderPipeline& Assets::GetShader(ShaderAsset key) {
    const auto entry = std::as_const(state.shaders).find(key);
    ASSERT(entry != state.shaders.cend(), "Shader not found: %u", static_cast<uint32_t>(key));
    return entry->second;
}

LLGL::Shader* Assets::GetComputeShader(ComputeShaderAsset key) {
    const auto entry = std::as_const(state.compute_shaders).find(key);
    ASSERT(entry != state.compute_shaders.cend(), "ComputeShader not found: %u", static_cast<uint32_t>(key));
    return entry->second;
}

const Sampler& Assets::GetSampler(size_t index) {
    ASSERT(index < state.samplers.size(), "Index is out of bounds: %zu", index);
    return state.samplers[index];
}

const LLGL::VertexFormat& Assets::GetVertexFormat(VertexFormatAsset key) {
    const auto entry = std::as_const(state.vertex_formats).find(key);
    ASSERT(entry != state.vertex_formats.cend(), "VertexFormat not found: %u", static_cast<uint32_t>(key));
    return entry->second;
}

static bool load_texture(const char* path, int sampler, Texture* texture) {
    int width, height;

    uint8_t* data = stbi_load(path, &width, &height, nullptr, 4);
    if (data == nullptr) {
        LOG_ERROR("Couldn't load asset: %s", path);
        return false;
    }

    *texture = Engine::Renderer().CreateTexture(LLGL::TextureType::Texture2D, LLGL::ImageFormat::RGBA, width, height, 1, sampler, data);

    stbi_image_free(data);

    return true;
}

template <size_t T>
Texture load_texture_array(const std::array<std::tuple<uint16_t, TextureAsset, const char*>, T>& assets, int sampler, bool generate_mip_maps) {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t layers_count = 0;

    struct Layer {
        uint8_t* data;
        uint32_t cols;
        uint32_t rows;
    };

    std::unordered_map<uint16_t, Layer> layer_to_data_map;
    layer_to_data_map.reserve(assets.size());

    for (const auto& [block_type, asset_key, path] : assets) {
        layers_count = glm::max(layers_count, static_cast<uint32_t>(block_type));

        int w, h;
        uint8_t* layer_data = stbi_load(path, &w, &h, nullptr, 4);
        if (layer_data == nullptr) {
            LOG_ERROR("Couldn't load asset: %s", path);
            continue;
        }

        layer_to_data_map[block_type] = Layer {
            .data = layer_data,
            .cols = static_cast<uint32_t>(w),
            .rows = static_cast<uint32_t>(h),
        };

        width = glm::max(width, static_cast<uint32_t>(w));
        height = glm::max(height, static_cast<uint32_t>(h));
    }
    layers_count += 1;

    const size_t data_size = width * height * layers_count * 4;
    uint8_t* image_data = new uint8_t[data_size];
    memset(image_data, 0, data_size);

    for (const auto& [layer, layer_data] : layer_to_data_map) {
        for (size_t row = 0; row < layer_data.rows; ++row) {
            memcpy(&image_data[width * height * 4 * layer + row * width * 4], &layer_data.data[row * layer_data.cols * 4], layer_data.cols * 4);
        }
        stbi_image_free(layer_data.data);
    }
    
    return Engine::Renderer().CreateTexture(LLGL::TextureType::Texture2DArray, LLGL::ImageFormat::RGBA, width, height, layers_count, sampler, image_data, generate_mip_maps);
}

template <typename T>
static T read(std::ifstream& file) {
    T data;
    file.read(reinterpret_cast<char*>(&data), sizeof(T));
    return data;
}

static bool load_font(Font& font, const char* meta_file_path, const char* atlas_file_path) {
    std::ifstream meta_file(meta_file_path, std::ios::in | std::ios::binary);

    if (!meta_file.good()) {
        LOG_ERROR("Failed to open file %s", meta_file_path);
        return false;
    }

    const FT_UShort ascender = read<FT_UShort>(meta_file);
    const uint32_t font_size = read<uint32_t>(meta_file);
    const uint32_t texture_width = read<uint32_t>(meta_file);
    const uint32_t texture_height = read<uint32_t>(meta_file);
    const glm::vec2 texture_size = glm::vec2(texture_width, texture_height);

    const uint32_t num_glyphs = read<uint32_t>(meta_file);

    for (uint32_t i = 0; i < num_glyphs; ++i) {
        FT_ULong c = read<FT_ULong>(meta_file);
        uint32_t bitmap_width = read<uint32_t>(meta_file);
        uint32_t bitmap_rows = read<uint32_t>(meta_file);
        FT_Int bitmap_left = read<FT_Int>(meta_file);
        FT_Int bitmap_top = read<FT_Int>(meta_file);
        FT_Pos advance_x = read<FT_Pos>(meta_file);
        uint32_t col = read<uint32_t>(meta_file);
        uint32_t row = read<uint32_t>(meta_file);

        const glm::ivec2 size = glm::ivec2(bitmap_width, bitmap_rows);
        
        const Glyph glyph = {
            .size = size,
            .tex_size = glm::vec2(size) / texture_size,
            .bearing = glm::ivec2(bitmap_left, bitmap_top),
            .advance = advance_x,
            .texture_coords = glm::vec2(col, row) / texture_size
        };

        font.glyphs[c] = glyph;
    }

    meta_file.close();

    int w, h;
    stbi_uc* data = stbi_load(atlas_file_path, &w, &h, nullptr, 1);
    font.texture = Engine::Renderer().CreateTexture(LLGL::TextureType::Texture2D, LLGL::ImageFormat::R, texture_width, texture_height, 1, TextureSampler::Linear, data);
    stbi_image_free(data);

    font.font_size = font_size;
    font.ascender = ascender >> 6;

    return true;
}