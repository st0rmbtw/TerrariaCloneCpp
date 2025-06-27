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

#include <SGE/engine.hpp>
#include <SGE/types/shader_pipeline.hpp>
#include <SGE/types/texture.hpp>
#include <SGE/types/attributes.hpp>
#include <SGE/log.hpp>
#include <SGE/utils/io.hpp>
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

    explicit AssetTexture(std::string path, int sampler = sge::TextureSampler::Nearest) :
        path(std::move(path)),
        sampler(sampler) {}
};

namespace ShaderStages {
    enum : uint8_t {
        Vertex = 1 << 0,
        Fragment = 1 << 1,
        Geometry = 1 << 2,
        Compute = 1 << 3,
    };
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

    { TextureAsset::UiCursorForeground,    AssetTexture("assets/sprites/ui/Cursor_0.png", sge::TextureSampler::Linear) },
    { TextureAsset::UiCursorBackground,    AssetTexture("assets/sprites/ui/Cursor_11.png", sge::TextureSampler::Linear) },
    { TextureAsset::UiInventoryBackground, AssetTexture("assets/sprites/ui/Inventory_Back.png", sge::TextureSampler::Linear) },
    { TextureAsset::UiInventorySelected,   AssetTexture("assets/sprites/ui/Inventory_Back14.png", sge::TextureSampler::Linear) },
    { TextureAsset::UiInventoryHotbar,     AssetTexture("assets/sprites/ui/Inventory_Back9.png", sge::TextureSampler::Linear) },

    { TextureAsset::TileCracks, AssetTexture("assets/sprites/tiles/TileCracks.png", sge::TextureSampler::Nearest) },

    { TextureAsset::Particles, AssetTexture("assets/sprites/Particles.png") },

    { TextureAsset::Flames0, AssetTexture("assets/sprites/Flame_0.png") }
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

    { TextureAsset::TileCracks,         AssetTextureAtlas(6, 4, glm::uvec2(16)) },

    { TextureAsset::Flames0,            AssetTextureAtlas(6, 24, glm::uvec2(20), glm::uvec2(0, 2)) }
};

#define BLOCK_ASSET(BLOCK_TYPE, TEXTURE_ASSET, PATH, SIZE) std::make_tuple(static_cast<uint16_t>(BLOCK_TYPE), TEXTURE_ASSET, PATH, SIZE)
static const std::array BLOCK_ASSETS = {
    BLOCK_ASSET(TileType::Dirt, TextureAsset::Tiles0, "assets/sprites/tiles/Tiles_0.png", glm::uvec2(16)),
    BLOCK_ASSET(TileType::Stone, TextureAsset::Tiles1, "assets/sprites/tiles/Tiles_1.png", glm::uvec2(16)),
    BLOCK_ASSET(TileType::Grass, TextureAsset::Tiles2, "assets/sprites/tiles/Tiles_2.png", glm::uvec2(16)),
    BLOCK_ASSET(TileType::Torch, TextureAsset::Tiles4, "assets/sprites/tiles/Tiles_4.png", glm::uvec2(20)),
    BLOCK_ASSET(TileType::Wood, TextureAsset::Tiles30, "assets/sprites/tiles/Tiles_30.png", glm::uvec2(16)),
};

#define WALL_ASSET(WALL_TYPE, PATH) std::make_tuple(static_cast<uint16_t>(WALL_TYPE), TextureAsset::Stub, PATH, glm::uvec2(32))
static const std::array WALL_ASSETS = {
    WALL_ASSET(WallType::DirtWall, "assets/sprites/walls/Wall_2.png"),
    WALL_ASSET(WallType::StoneWall, "assets/sprites/walls/Wall_1.png"),
    WALL_ASSET(WallType::WoodWall, "assets/sprites/walls/Wall_4.png"),
};

#define BACKGROUND_ASSET(BACKGROUND_ASSET, PATH) std::make_tuple(static_cast<uint16_t>(BACKGROUND_ASSET), TextureAsset::Stub, PATH, glm::uvec2(0))
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
    { ShaderAsset::ParticleShader,       AssetShader("particle",     ShaderStages::Vertex | ShaderStages::Fragment, { VertexFormatAsset::ParticleVertex,  VertexFormatAsset::ParticleInstance  }) },
    { ShaderAsset::StaticLightMapShader, AssetShader("lightmap",     ShaderStages::Vertex | ShaderStages::Fragment, VertexFormatAsset::StaticLightMapVertex ) },
};

const std::pair<ComputeShaderAsset, AssetComputeShader> COMPUTE_SHADER_ASSETS[] = {
    { ComputeShaderAsset::ParticleComputeTransformShader, AssetComputeShader("particle", "CSComputeTransform", "particle") },
    { ComputeShaderAsset::LightVertical, AssetComputeShader("light", "CSComputeLightVertical", "light_blur_vertical") },
    { ComputeShaderAsset::LightHorizontal, AssetComputeShader("light", "CSComputeLightHorizontal", "light_blur_horizontal") },
    { ComputeShaderAsset::LightSetLightSources, AssetComputeShader("light", "CSComputeLightSetLightSources", "light_init") },
};

static struct AssetsState {
    std::unordered_map<uint16_t, sge::Texture> items;
    std::unordered_map<TextureAsset, sge::Texture> textures;
    std::unordered_map<TextureAsset, sge::TextureAtlas> textures_atlases;
    std::unordered_map<ShaderAsset, sge::ShaderPipeline> shaders;
    std::unordered_map<ComputeShaderAsset, LLGL::Shader*> compute_shaders;
    std::unordered_map<FontAsset, sge::Font> fonts;
    std::unordered_map<VertexFormatAsset, LLGL::VertexFormat> vertex_formats;
    std::vector<sge::Sampler> samplers;
} state;

static bool load_font(sge::Font& font, const char* meta_file_path, const char* atlas_file_path);
static bool load_texture(const char* path, int sampler, sge::Texture* texture);

template <size_t T>
static sge::Texture load_texture_array(const std::array<std::tuple<uint16_t, TextureAsset, const char*, glm::uvec2>, T>& assets, int sampler, bool generate_mip_maps = false);

void InitSamplers() {
    sge::Renderer& renderer = sge::Engine::Renderer();
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

        state.samplers[sge::TextureSampler::Linear] = sge::Sampler(context->CreateSampler(sampler_desc), sampler_desc);
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

        state.samplers[sge::TextureSampler::LinearMips] = sge::Sampler(context->CreateSampler(sampler_desc), sampler_desc);
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

        state.samplers[sge::TextureSampler::Nearest] = sge::Sampler(context->CreateSampler(sampler_desc), sampler_desc);
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

        state.samplers[sge::TextureSampler::NearestMips] = sge::Sampler(context->CreateSampler(sampler_desc), sampler_desc);
    }
}

bool Assets::Load() {
    InitSamplers();

    sge::Renderer& renderer = sge::Engine::Renderer();

    const uint8_t data[] = { 0xFF, 0xFF, 0xFF, 0xFF };
    state.textures[TextureAsset::Stub] = renderer.CreateTexture(LLGL::TextureType::Texture2D, LLGL::ImageFormat::RGBA, 1, 1, 1, Assets::GetSampler(sge::TextureSampler::Nearest), data);

    for (const auto& [key, asset] : TEXTURE_ASSETS) {
        sge::Texture texture;
        if (!load_texture(asset.path.c_str(), asset.sampler, &texture)) {
            return false;
        }
        state.textures[key] = texture;
    }

    for (const auto& [block_type, asset_key, path, size] : BLOCK_ASSETS) {
        sge::Texture texture;
        if (!load_texture(path, sge::TextureSampler::Nearest, &texture)) {
            return false;
        }
        state.textures[asset_key] = texture;
        state.textures_atlases[asset_key] = sge::TextureAtlas::from_grid(texture, size, (texture.width() - (texture.width() / 16) * 2) / 16 + 1, (texture.height() - (texture.height() / 16) * 2) / 16 + 1, glm::uvec2(2));
    }

    std::vector<sge::Rect> background_rects(static_cast<size_t>(BackgroundAsset::Count));
    for (const auto& [id, asset_key, path, _] : BACKGROUND_ASSETS) {
        int width;
        int height;

        stbi_info(path, &width, &height, nullptr);

        background_rects[id] = sge::URect::from_top_left(glm::uvec2(0), glm::uvec2(width, height));
    }
    state.textures_atlases[TextureAsset::Backgrounds] = sge::TextureAtlas(Assets::GetTexture(TextureAsset::Stub), std::move(background_rects), glm::vec2(0.0f), 0, 0);

    for (const auto& [key, asset] : TEXTURE_ATLAS_ASSETS) {
        state.textures_atlases[key] = sge::TextureAtlas::from_grid(Assets::GetTexture(key), asset.tile_size, asset.columns, asset.rows, asset.padding, asset.offset);
    }

    for (const auto& [key, asset] : ITEM_ASSETS) {
        sge::Texture texture;
        if (!load_texture(asset.c_str(), sge::TextureSampler::Nearest, &texture)) {
            return false;
        }
        state.items[key] = texture;
    }

    // There is some glitches in mipmaps on Metal
    const bool mip_maps = !renderer.Backend().IsMetal();

    state.textures[TextureAsset::Tiles] = load_texture_array(BLOCK_ASSETS, mip_maps ? sge::TextureSampler::NearestMips : sge::TextureSampler::Nearest, mip_maps);
    state.textures[TextureAsset::Walls] = load_texture_array(WALL_ASSETS, sge::TextureSampler::Nearest);
    state.textures[TextureAsset::Backgrounds] = load_texture_array(BACKGROUND_ASSETS, sge::TextureSampler::Nearest);

    return true;
}

bool Assets::LoadShaders(const std::vector<sge::ShaderDef>& shader_defs) {
    InitVertexFormats();

    sge::Renderer& renderer = sge::Engine::Renderer();

    sge::RenderBackend backend = renderer.Backend();

    for (const auto& [key, asset] : SHADER_ASSETS) {
        sge::ShaderPipeline shader_pipeline;

        std::vector<LLGL::VertexAttribute> attributes;

        for (const VertexFormatAsset asset : asset.vertex_format_assets) {
            const LLGL::VertexFormat& vertex_format = Assets::GetVertexFormat(asset);
            attributes.insert(attributes.end(), vertex_format.attributes.begin(), vertex_format.attributes.end());
        }

        if (BITFLAG_CHECK(asset.stages, ShaderStages::Vertex)) {
            if (!(shader_pipeline.vs = renderer.LoadShader(sge::ShaderPath(sge::ShaderType::Vertex, asset.file_name), shader_defs, attributes)))
                return false;
        }

        if (BITFLAG_CHECK(asset.stages, ShaderStages::Fragment)) {
            if (!(shader_pipeline.ps = renderer.LoadShader(sge::ShaderPath(sge::ShaderType::Fragment, asset.file_name), shader_defs)))
                return false;
        }

        if (BITFLAG_CHECK(asset.stages, ShaderStages::Geometry)) {
            if (!(shader_pipeline.gs = renderer.LoadShader(sge::ShaderPath(sge::ShaderType::Geometry, asset.file_name), shader_defs)))
                return false;
        }

        state.shaders[key] = shader_pipeline;
    }

    for (const auto& [key, asset] : COMPUTE_SHADER_ASSETS) {
        const std::string& file_name = backend.IsGLSL() ? asset.glsl_file_name : asset.file_name;

        if (!(state.compute_shaders[key] = renderer.LoadShader(sge::ShaderPath(sge::ShaderType::Compute, file_name, asset.func_name), shader_defs)))
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

        if (!sge::FileExists(meta_file_str.c_str())) {
            SGE_LOG_ERROR("Failed to find the font meta file '{}'", meta_file_str.c_str());
            return false;
        }

        if (!sge::FileExists(atlas_file_str.c_str())) {
            SGE_LOG_ERROR("Failed to find the font atlas file '{}'", atlas_file_str.c_str());
            return false;
        }

        sge::Font font;
        if (!load_font(font, meta_file_str.c_str(), atlas_file_str.c_str())) return false;

        state.fonts[key] = font;
    }

    return true;
}

void Assets::InitVertexFormats() {
    sge::Renderer& renderer = sge::Engine::Renderer();

    const sge::RenderBackend backend = renderer.Backend();

    LLGL::VertexFormat tilemap_vertex_format;
    LLGL::VertexFormat tilemap_instance_format;
    LLGL::VertexFormat background_vertex_format;
    LLGL::VertexFormat background_instance_format;
    LLGL::VertexFormat particle_vertex_format;
    LLGL::VertexFormat particle_instance_format;
    LLGL::VertexFormat postprocess_vertex_format;
    LLGL::VertexFormat static_lightmap_vertex_format;

    tilemap_vertex_format.attributes = sge::Attributes({
        sge::Attribute::Vertex(LLGL::Format::RG32Float, "a_position", "Position"),
    }).ToLLGL(backend);

    tilemap_instance_format.attributes = sge::Attributes({
        sge::Attribute::Instance(LLGL::Format::R16UInt, "i_position", "I_Position", 1),
        sge::Attribute::Instance(LLGL::Format::RG32Float, "i_atlas_pos", "I_AtlasPos", 1),
        sge::Attribute::Instance(LLGL::Format::RG32Float, "i_world_pos", "I_WorldPos", 1),
        sge::Attribute::Instance(LLGL::Format::R32UInt, "i_tile_data", "I_TileData", 1),
    }).ToLLGL(backend, 1);

    background_vertex_format.attributes = sge::Attributes({
        sge::Attribute::Vertex(LLGL::Format::RG32Float, "a_position", "Position"),
        sge::Attribute::Vertex(LLGL::Format::RG32Float, "a_texture_size", "TextureSize")
    }).ToLLGL(backend);

    background_instance_format.attributes = sge::Attributes({
        sge::Attribute::Instance(LLGL::Format::RG32Float, "i_position", "I_Position", 1),
        sge::Attribute::Instance(LLGL::Format::RG32Float, "i_size", "I_Size", 1),
        sge::Attribute::Instance(LLGL::Format::RG32Float, "i_tex_size", "I_TexSize", 1),
        sge::Attribute::Instance(LLGL::Format::RG32Float, "i_speed", "I_Speed", 1),
        sge::Attribute::Instance(LLGL::Format::R32UInt, "i_id", "I_ID", 1),
        sge::Attribute::Instance(LLGL::Format::R32SInt, "i_flags", "I_Flags", 1),
    }).ToLLGL(backend, 2);

    particle_vertex_format.attributes = sge::Attributes({
        sge::Attribute::Vertex(LLGL::Format::RG32Float, "a_position", "Position"),
        sge::Attribute::Vertex(LLGL::Format::RG32Float, "a_inv_tex_size", "InvTexSize"),
        sge::Attribute::Vertex(LLGL::Format::RG32Float, "a_tex_size", "TexSize"),
    }).ToLLGL(backend);

    particle_instance_format.attributes = sge::Attributes({
        sge::Attribute::Instance(LLGL::Format::RG32Float, "i_uv", "I_UV", 1),
        sge::Attribute::Instance(LLGL::Format::R32Float, "i_depth", "I_Depth", 1),
        sge::Attribute::Instance(LLGL::Format::R32UInt, "i_id", "I_ID", 1),
        sge::Attribute::Instance(LLGL::Format::R32UInt, "I_is_world", "I_IsWorld", 1),
    }).ToLLGL(backend, 3);

    postprocess_vertex_format.attributes = sge::Attributes({
        sge::Attribute::Vertex(LLGL::Format::RG32Float, "a_position", "Position"),
        sge::Attribute::Vertex(LLGL::Format::RG32Float, "a_uv", "UV"),
        sge::Attribute::Vertex(LLGL::Format::RG32Float, "a_world_size", "WorldSize"),
    }).ToLLGL(backend);

    static_lightmap_vertex_format.attributes = sge::Attributes({
        sge::Attribute::Vertex(LLGL::Format::RG32Float, "a_position", "Position"),
        sge::Attribute::Vertex(LLGL::Format::RG32Float, "a_uv", "UV"),
    }).ToLLGL(backend);

    state.vertex_formats[VertexFormatAsset::TilemapVertex] = tilemap_vertex_format;
    state.vertex_formats[VertexFormatAsset::TilemapInstance] = tilemap_instance_format;
    state.vertex_formats[VertexFormatAsset::BackgroundVertex] = background_vertex_format;
    state.vertex_formats[VertexFormatAsset::BackgroundInstance] = background_instance_format;
    state.vertex_formats[VertexFormatAsset::ParticleVertex] = particle_vertex_format;
    state.vertex_formats[VertexFormatAsset::ParticleInstance] = particle_instance_format;
    state.vertex_formats[VertexFormatAsset::PostProcessVertex] = postprocess_vertex_format;
    state.vertex_formats[VertexFormatAsset::StaticLightMapVertex] = static_lightmap_vertex_format;
}

void Assets::DestroyTextures() {
    const auto& context = sge::Engine::Renderer().Context();

    for (auto& entry : state.textures) {
        context->Release(entry.second);
    }
}

void Assets::DestroySamplers() {
    const auto& context = sge::Engine::Renderer().Context();

    for (auto& sampler : state.samplers) {
        context->Release(sampler);
    }
}

void Assets::DestroyShaders() {
    const auto& context = sge::Engine::Renderer().Context();

    for (auto& entry : state.shaders) {
        entry.second.Unload(context);
    }
}

const sge::Texture& Assets::GetTexture(TextureAsset key) {
    const auto entry = std::as_const(state.textures).find(key);
    SGE_ASSERT(entry != state.textures.cend());
    return entry->second;
}

const sge::TextureAtlas& Assets::GetTextureAtlas(TextureAsset key) {
    const auto entry = std::as_const(state.textures_atlases).find(key);
    SGE_ASSERT(entry != state.textures_atlases.cend());
    return entry->second;
}

const sge::Font& Assets::GetFont(FontAsset key) {
    const auto entry = std::as_const(state.fonts).find(key);
    SGE_ASSERT(entry != state.fonts.cend());
    return entry->second;
}

const sge::Texture& Assets::GetItemTexture(uint16_t index) {
    const auto entry = std::as_const(state.items).find(index);
    SGE_ASSERT(entry != state.items.cend());
    return entry->second;
}

const sge::ShaderPipeline& Assets::GetShader(ShaderAsset key) {
    const auto entry = std::as_const(state.shaders).find(key);
    SGE_ASSERT(entry != state.shaders.cend());
    return entry->second;
}

LLGL::Shader* Assets::GetComputeShader(ComputeShaderAsset key) {
    const auto entry = std::as_const(state.compute_shaders).find(key);
    SGE_ASSERT(entry != state.compute_shaders.cend());
    return entry->second;
}

const sge::Sampler& Assets::GetSampler(size_t index) {
    SGE_ASSERT(index < state.samplers.size());
    return state.samplers[index];
}

const LLGL::VertexFormat& Assets::GetVertexFormat(VertexFormatAsset key) {
    const auto entry = std::as_const(state.vertex_formats).find(key);
    SGE_ASSERT(entry != state.vertex_formats.cend());
    return entry->second;
}

static bool load_texture(const char* path, int sampler, sge::Texture* texture) {
    int width, height;

    uint8_t* data = stbi_load(path, &width, &height, nullptr, 4);
    if (data == nullptr) {
        SGE_LOG_ERROR("Couldn't load asset: {}", path);
        return false;
    }

    const bool generate_mips = sampler == sge::TextureSampler::NearestMips || sampler == sge::TextureSampler::LinearMips;
    *texture = sge::Engine::Renderer().CreateTexture(LLGL::TextureType::Texture2D, LLGL::ImageFormat::RGBA, width, height, 1, Assets::GetSampler(sampler), data, generate_mips);

    stbi_image_free(data);

    return true;
}

template <size_t T>
sge::Texture load_texture_array(const std::array<std::tuple<uint16_t, TextureAsset, const char*, glm::uvec2>, T>& assets, int sampler, bool generate_mip_maps) {
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

    for (const auto& [block_type, asset_key, path, _] : assets) {
        layers_count = glm::max(layers_count, static_cast<uint32_t>(block_type));

        int w, h;
        uint8_t* layer_data = stbi_load(path, &w, &h, nullptr, 4);
        if (layer_data == nullptr) {
            SGE_LOG_ERROR("Couldn't load asset: {}", path);
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

    LLGL::DynamicArray<uint8_t> pixels(width * height * layers_count * 4);

    for (const auto& [layer, layer_data] : layer_to_data_map) {
        for (size_t row = 0; row < layer_data.rows; ++row) {
            memcpy(&pixels[width * height * 4 * layer + row * width * 4], &layer_data.data[row * layer_data.cols * 4], layer_data.cols * 4);
        }
        stbi_image_free(layer_data.data);
    }

    return sge::Engine::Renderer().CreateTexture(LLGL::TextureType::Texture2DArray, LLGL::ImageFormat::RGBA, LLGL::DataType::UInt8, width, height, layers_count, Assets::GetSampler(sampler), pixels.data(), generate_mip_maps);
}

template <typename T>
static T read(std::ifstream& file) {
    T data;
    file.read(reinterpret_cast<char*>(&data), sizeof(T));
    return data;
}

static bool load_font(sge::Font& font, const char* meta_file_path, const char* atlas_file_path) {
    std::ifstream meta_file(meta_file_path, std::ios::in | std::ios::binary);

    if (!meta_file.good()) {
        SGE_LOG_ERROR("Failed to open file {}", meta_file_path);
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

        const sge::Glyph glyph = {
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

    const sge::Sampler& linear_sampler = Assets::GetSampler(sge::TextureSampler::Linear);
    font.texture = sge::Engine::Renderer().CreateTexture(LLGL::TextureType::Texture2D, LLGL::ImageFormat::R, LLGL::DataType::UInt8, texture_width, texture_height, 1, linear_sampler, data);
    stbi_image_free(data);

    font.font_size = font_size;
    font.ascender = ascender >> 6;

    return true;
}
