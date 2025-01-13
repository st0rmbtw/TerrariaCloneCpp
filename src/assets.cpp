#include "assets.hpp"

#include <array>

#include <LLGL/Utils/VertexFormat.h>
#include <LLGL/Shader.h>
#include <LLGL/ShaderFlags.h>
#include <LLGL/VertexAttribute.h>
#include <LLGL/TextureFlags.h>
#include <STB/stb_image.h>
#include <ft2build.h>
#include <freetype/freetype.h>

#include "LLGL/Format.h"
#include "renderer/renderer.hpp"
#include "renderer/types.hpp"
#include "log.hpp"
#include "utils.hpp"
#include "types/shader_pipeline.hpp"
#include "types/shader_type.hpp"
#include "types/texture.hpp"

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
    enum : uint8_t {
        Vertex = 1 << 0,
        Fragment = 1 << 1,
        Geometry = 1 << 2,
        Compute = 1 << 3
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

static const std::array BLOCK_ASSETS = {
    std::make_tuple(static_cast<uint16_t>(BlockType::Dirt), TextureAsset::Tiles0, "assets/sprites/tiles/Tiles_0.png"),
    std::make_tuple(static_cast<uint16_t>(BlockType::Stone), TextureAsset::Tiles1, "assets/sprites/tiles/Tiles_1.png"),
    std::make_tuple(static_cast<uint16_t>(BlockType::Grass), TextureAsset::Tiles2, "assets/sprites/tiles/Tiles_2.png"),
    std::make_tuple(static_cast<uint16_t>(BlockType::Wood), TextureAsset::Tiles30, "assets/sprites/tiles/Tiles_30.png"),
};

static const std::array WALL_ASSETS = {
    std::make_tuple(static_cast<uint16_t>(WallType::DirtWall), TextureAsset::Stub, "assets/sprites/walls/Wall_2.png"),
    std::make_tuple(static_cast<uint16_t>(WallType::StoneWall), TextureAsset::Stub, "assets/sprites/walls/Wall_1.png"),
};

static const std::array BACKGROUND_ASSETS = {
    std::make_tuple(static_cast<uint16_t>(BackgroundAsset::Background0), TextureAsset::Stub, "assets/sprites/backgrounds/Background_0.png"),
    std::make_tuple(static_cast<uint16_t>(BackgroundAsset::Background7), TextureAsset::Stub, "assets/sprites/backgrounds/Background_7.png"),
    std::make_tuple(static_cast<uint16_t>(BackgroundAsset::Background55), TextureAsset::Stub, "assets/sprites/backgrounds/Background_55.png"),
    std::make_tuple(static_cast<uint16_t>(BackgroundAsset::Background74), TextureAsset::Stub, "assets/sprites/backgrounds/Background_74.png"),
    std::make_tuple(static_cast<uint16_t>(BackgroundAsset::Background77), TextureAsset::Stub, "assets/sprites/backgrounds/Background_77.png"),
    std::make_tuple(static_cast<uint16_t>(BackgroundAsset::Background78), TextureAsset::Stub, "assets/sprites/backgrounds/Background_78.png"),
    std::make_tuple(static_cast<uint16_t>(BackgroundAsset::Background90), TextureAsset::Stub, "assets/sprites/backgrounds/Background_90.png"),
    std::make_tuple(static_cast<uint16_t>(BackgroundAsset::Background91), TextureAsset::Stub, "assets/sprites/backgrounds/Background_91.png"),
    std::make_tuple(static_cast<uint16_t>(BackgroundAsset::Background92), TextureAsset::Stub, "assets/sprites/backgrounds/Background_92.png"),
    std::make_tuple(static_cast<uint16_t>(BackgroundAsset::Background93), TextureAsset::Stub, "assets/sprites/backgrounds/Background_93.png"),
    std::make_tuple(static_cast<uint16_t>(BackgroundAsset::Background112), TextureAsset::Stub, "assets/sprites/backgrounds/Background_112.png"),
    std::make_tuple(static_cast<uint16_t>(BackgroundAsset::Background114), TextureAsset::Stub, "assets/sprites/backgrounds/Background_114.png"),
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

static const std::array FONT_ASSETS = {
    std::make_pair(FontAsset::AndyBold, "assets/fonts/andy_bold.otf"),
    std::make_pair(FontAsset::AndyRegular, "assets/fonts/andy_regular.otf"),
};

const std::pair<ShaderAsset, AssetShader> SHADER_ASSETS[] = {
    { ShaderAsset::BackgroundShader, AssetShader("background", ShaderStages::Vertex | ShaderStages::Fragment, { VertexFormatAsset::BackgroundVertex, VertexFormatAsset::BackgroundInstance }) },
    { ShaderAsset::PostProcessShader,AssetShader("postprocess",ShaderStages::Vertex | ShaderStages::Fragment, VertexFormatAsset::PostProcessVertex) },
    { ShaderAsset::TilemapShader,    AssetShader("tilemap",    ShaderStages::Vertex | ShaderStages::Fragment, { VertexFormatAsset::TilemapVertex,  VertexFormatAsset::TilemapInstance  }) },
    { ShaderAsset::FontShader,       AssetShader("font",       ShaderStages::Vertex | ShaderStages::Fragment, { VertexFormatAsset::FontVertex,     VertexFormatAsset::FontInstance     }) },
    { ShaderAsset::SpriteShader,     AssetShader("sprite",     ShaderStages::Vertex | ShaderStages::Fragment, { VertexFormatAsset::SpriteVertex,   VertexFormatAsset::SpriteInstance   }) },
    { ShaderAsset::ParticleShader,   AssetShader("particle",   ShaderStages::Vertex | ShaderStages::Fragment, { VertexFormatAsset::ParticleVertex, VertexFormatAsset::ParticleInstance }) },
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

static bool load_font(FT_Library ft, const std::string& path, Font& font);
static bool load_texture(const char* path, int sampler, Texture* texture);

template <size_t T>
static Texture load_texture_array(const std::array<std::tuple<uint16_t, TextureAsset, const char*>, T>& assets, int sampler, bool generate_mip_maps = false);

bool Assets::Load() {
    const uint8_t data[] = { 0xFF, 0xFF, 0xFF, 0xFF };
    state.textures[TextureAsset::Stub] = Renderer::CreateTexture(LLGL::TextureType::Texture2D, LLGL::ImageFormat::RGBA, 1, 1, 1, TextureSampler::Nearest, data);

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
    state.textures_atlases[TextureAsset::Backgrounds] = TextureAtlas(Assets::GetTexture(TextureAsset::Stub), background_rects, glm::vec2(0.0f), 0, 0);

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
    const bool mip_maps = !Renderer::Backend().IsMetal();

    state.textures[TextureAsset::Tiles] = load_texture_array(BLOCK_ASSETS, mip_maps ? TextureSampler::NearestMips : TextureSampler::Nearest, mip_maps);
    state.textures[TextureAsset::Walls] = load_texture_array(WALL_ASSETS, TextureSampler::Nearest);
    state.textures[TextureAsset::Backgrounds] = load_texture_array(BACKGROUND_ASSETS, TextureSampler::Nearest);

    return true;
}

bool Assets::LoadShaders(const std::vector<ShaderDef>& shader_defs) {
    InitVertexFormats();

    RenderBackend backend = Renderer::Backend();

    for (const auto& [key, asset] : SHADER_ASSETS) {
        ShaderPipeline shader_pipeline;

        std::vector<LLGL::VertexAttribute> attributes;

        for (const VertexFormatAsset asset : asset.vertex_format_assets) {
            const LLGL::VertexFormat& vertex_format = Assets::GetVertexFormat(asset);
            attributes.insert(attributes.end(), vertex_format.attributes.begin(), vertex_format.attributes.end());
        }

        if ((asset.stages & ShaderStages::Vertex) == ShaderStages::Vertex) {
            if (!(shader_pipeline.vs = Renderer::LoadShader(ShaderPath(ShaderType::Vertex, asset.file_name), shader_defs, attributes)))
                return false;
        }
        
        if ((asset.stages & ShaderStages::Fragment) == ShaderStages::Fragment) {
            if (!(shader_pipeline.ps = Renderer::LoadShader(ShaderPath(ShaderType::Fragment, asset.file_name), shader_defs)))
                return false;
        }

        if ((asset.stages & ShaderStages::Geometry) == ShaderStages::Geometry) {
            if (!(shader_pipeline.gs = Renderer::LoadShader(ShaderPath(ShaderType::Geometry, asset.file_name), shader_defs)))
                return false;
        }

        state.shaders[key] = shader_pipeline;
    }

    for (const auto& [key, asset] : COMPUTE_SHADER_ASSETS) {
        const std::string& file_name = backend.IsGLSL() ? asset.glsl_file_name : asset.file_name;

        if (!(state.compute_shaders[key] = Renderer::LoadShader(ShaderPath(ShaderType::Compute, file_name, asset.func_name), shader_defs)))
            return false;
    }

    return true;
};

bool Assets::LoadFonts() {
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        LOG_ERROR("Couldn't init FreeType library.");
        return false;
    }

    for (const auto& [key, path] : FONT_ASSETS) {
        if (!FileExists(path)) {
            LOG_ERROR("Failed to find font '%s'",  path);
            return false;
        }

        Font font;
        if (!load_font(ft, path, font)) return false;

        state.fonts[key] = font;
    }

    FT_Done_FreeType(ft);

    return true;
}

bool Assets::InitSamplers() {
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

        state.samplers[TextureSampler::Linear] = Sampler(Renderer::Context()->CreateSampler(sampler_desc), sampler_desc);
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

        state.samplers[TextureSampler::LinearMips] = Sampler(Renderer::Context()->CreateSampler(sampler_desc), sampler_desc);
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

        state.samplers[TextureSampler::Nearest] = Sampler(Renderer::Context()->CreateSampler(sampler_desc), sampler_desc);
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

        state.samplers[TextureSampler::NearestMips] = Sampler(Renderer::Context()->CreateSampler(sampler_desc), sampler_desc);
    }

    return true;
}

void Assets::InitVertexFormats() {
    const RenderBackend backend = Renderer::Backend();

    LLGL::VertexFormat sprite_vertex_format;
    LLGL::VertexFormat sprite_instance_format;
    LLGL::VertexFormat tilemap_vertex_format;
    LLGL::VertexFormat tilemap_instance_format;
    LLGL::VertexFormat font_vertex_format;
    LLGL::VertexFormat font_instance_format;
    LLGL::VertexFormat background_vertex_format;
    LLGL::VertexFormat background_instance_format;
    LLGL::VertexFormat particle_vertex_format;
    LLGL::VertexFormat particle_instance_format;
    LLGL::VertexFormat lightmap_vertex_format;

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
            {"i_is_ui",     LLGL::Format::R32SInt,    6, offsetof(GlyphInstance,is_ui),    sizeof(GlyphInstance), 1, 1}
        };
    } else if (backend.IsHLSL()) {
        font_instance_format.attributes = {
            {"I_Color",    LLGL::Format::RGB32Float, 1, offsetof(GlyphInstance,color),    sizeof(GlyphInstance), 1, 1},
            {"I_Position", LLGL::Format::RGB32Float, 2, offsetof(GlyphInstance,pos),      sizeof(GlyphInstance), 1, 1},
            {"I_Size",     LLGL::Format::RG32Float,  3, offsetof(GlyphInstance,size),     sizeof(GlyphInstance), 1, 1},
            {"I_TexSize",  LLGL::Format::RG32Float,  4, offsetof(GlyphInstance,tex_size), sizeof(GlyphInstance), 1, 1},
            {"I_UV",       LLGL::Format::RG32Float,  5, offsetof(GlyphInstance,uv),       sizeof(GlyphInstance), 1, 1},
            {"I_IsUI",     LLGL::Format::R32SInt,    6, offsetof(GlyphInstance,is_ui),    sizeof(GlyphInstance), 1, 1}
        };
    } else {
        font_instance_format.attributes = {
            {"i_color",     LLGL::Format::RGB32Float, 1, offsetof(GlyphInstance,color),    sizeof(GlyphInstance), 1, 1},
            {"i_position",  LLGL::Format::RGB32Float, 2, offsetof(GlyphInstance,pos),      sizeof(GlyphInstance), 1, 1},
            {"i_size",      LLGL::Format::RG32Float,  3, offsetof(GlyphInstance,size),     sizeof(GlyphInstance), 1, 1},
            {"i_tex_size",  LLGL::Format::RG32Float,  4, offsetof(GlyphInstance,tex_size), sizeof(GlyphInstance), 1, 1},
            {"i_uv",        LLGL::Format::RG32Float,  5, offsetof(GlyphInstance,uv),       sizeof(GlyphInstance), 1, 1},
            {"i_is_ui",     LLGL::Format::R32SInt,    6, offsetof(GlyphInstance,is_ui),    sizeof(GlyphInstance), 1, 1}
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
            { "i_uv",    LLGL::Format::RG32Float, 3, offsetof(ParticleInstance, uv),    sizeof(ParticleInstance), 1, 1},
            { "i_depth", LLGL::Format::R32Float,  4, offsetof(ParticleInstance, depth), sizeof(ParticleInstance), 1, 1}
        };
    } else if (backend.IsHLSL()) {
        particle_instance_format.attributes = {
            { "I_UV",    LLGL::Format::RG32Float, 3, offsetof(ParticleInstance, uv),    sizeof(ParticleInstance), 1, 1 },
            { "I_Depth", LLGL::Format::R32Float,  4, offsetof(ParticleInstance, depth), sizeof(ParticleInstance), 1, 1 }
        };
    } else {
        particle_instance_format.attributes = {
            { "i_uv",    LLGL::Format::RG32Float, 3, offsetof(ParticleInstance, uv),    sizeof(ParticleInstance), 1, 1 },
            { "i_depth", LLGL::Format::R32Float,  4, offsetof(ParticleInstance, depth), sizeof(ParticleInstance), 1, 1 },
        };
    }

    if (backend.IsGLSL()) {
        lightmap_vertex_format.AppendAttribute({ "a_position",   LLGL::Format::RG32Float });
        lightmap_vertex_format.AppendAttribute({ "a_uv",         LLGL::Format::RG32Float });
        lightmap_vertex_format.AppendAttribute({ "a_world_size", LLGL::Format::RG32Float });
    } else if (backend.IsHLSL()) {
        lightmap_vertex_format.AppendAttribute({ "Position",  LLGL::Format::RG32Float });
        lightmap_vertex_format.AppendAttribute({ "UV",        LLGL::Format::RG32Float });
        lightmap_vertex_format.AppendAttribute({ "WorldSize", LLGL::Format::RG32Float });
    } else {
        lightmap_vertex_format.AppendAttribute({ "position",  LLGL::Format::RG32Float });
        lightmap_vertex_format.AppendAttribute({ "uv",        LLGL::Format::RG32Float });
        lightmap_vertex_format.AppendAttribute({ "world_size", LLGL::Format::RG32Float });
    }

    state.vertex_formats[VertexFormatAsset::SpriteVertex] = sprite_vertex_format;
    state.vertex_formats[VertexFormatAsset::SpriteInstance] = sprite_instance_format;
    state.vertex_formats[VertexFormatAsset::TilemapVertex] = tilemap_vertex_format;
    state.vertex_formats[VertexFormatAsset::TilemapInstance] = tilemap_instance_format;
    state.vertex_formats[VertexFormatAsset::FontVertex] = font_vertex_format;
    state.vertex_formats[VertexFormatAsset::FontInstance] = font_instance_format;
    state.vertex_formats[VertexFormatAsset::BackgroundVertex] = background_vertex_format;
    state.vertex_formats[VertexFormatAsset::BackgroundInstance] = background_instance_format;
    state.vertex_formats[VertexFormatAsset::ParticleVertex] = particle_vertex_format;
    state.vertex_formats[VertexFormatAsset::ParticleInstance] = particle_instance_format;
    state.vertex_formats[VertexFormatAsset::PostProcessVertex] = lightmap_vertex_format;
}

void Assets::DestroyTextures() {
    for (auto& entry : state.textures) {
        Renderer::Context()->Release(entry.second);
    }
}

void Assets::DestroySamplers() {
    for (auto& sampler : state.samplers) {
        Renderer::Context()->Release(sampler);
    }
}

void Assets::DestroyShaders() {
    for (auto& entry : state.shaders) {
        entry.second.Unload(Renderer::Context());
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

const Texture& Assets::GetItemTexture(size_t index) {
    const auto entry = std::as_const(state.items).find(index);
    ASSERT(entry != state.items.cend(), "Item not found: %zu", index);
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

    *texture = Renderer::CreateTexture(LLGL::TextureType::Texture2D, LLGL::ImageFormat::RGBA, width, height, 1, sampler, data);

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
    
    return Renderer::CreateTexture(LLGL::TextureType::Texture2DArray, LLGL::ImageFormat::RGBA, width, height, layers_count, sampler, image_data, generate_mip_maps);
}

bool load_font(FT_Library ft, const std::string& path, Font& font) {
    static constexpr uint32_t FONT_SIZE = 68;
    static constexpr uint32_t PADDING = 4;

    FT_Face face;
    if (FT_New_Face(ft, path.c_str(), 0, &face)) {
        LOG_ERROR("Failed to load font: %s", path.c_str());
        return false;
    }
    
    FT_Set_Pixel_Sizes(face, 0, FONT_SIZE);

    struct GlyphInfo {
        uint8_t* buffer;
        uint32_t bitmap_width;
        uint32_t bitmap_rows;
        FT_Int bitmap_left;
        FT_Int bitmap_top;
        FT_Vector advance;
        uint32_t col;
        uint32_t row;
    };

    std::vector<std::pair<FT_ULong, GlyphInfo>> glyphs;

    FT_UInt index;
    FT_ULong character = FT_Get_First_Char(face, &index);

    const uint32_t texture_width = 1512;

    uint32_t col = PADDING;
    uint32_t row = PADDING;

    uint32_t max_height = 0;

    while (true) {
        if (FT_Load_Char(face, character, FT_LOAD_DEFAULT)) {
            LOG_ERROR("Failed to load glyph %lu", character);
        } else {
            FT_Render_Glyph(face->glyph, FT_RENDER_MODE_SDF);

            if (col + face->glyph->bitmap.width + PADDING >= texture_width) {
                col = PADDING;
                row += max_height + PADDING;
                max_height = 0;
            }

            max_height = std::max(max_height, face->glyph->bitmap.rows);

            GlyphInfo info;
            info.bitmap_width = face->glyph->bitmap.width;
            info.bitmap_rows = face->glyph->bitmap.rows;
            info.bitmap_left = face->glyph->bitmap_left;
            info.bitmap_top = face->glyph->bitmap_top;
            info.advance = face->glyph->advance;
            info.col = col;
            info.row = row;
            info.buffer = new uint8_t[face->glyph->bitmap.pitch * info.bitmap_rows];
            memcpy(info.buffer, face->glyph->bitmap.buffer, face->glyph->bitmap.pitch * info.bitmap_rows);

            glyphs.emplace_back(character, info);

            col += info.bitmap_width + PADDING;
        }

        character = FT_Get_Next_Char(face, character, &index);
        if (!index) break;
    }

    const uint32_t texture_height = row + max_height;

    const glm::vec2 texture_size = glm::vec2(texture_width, texture_height);

    uint8_t* texture_data = new uint8_t[texture_width * texture_height];
    memset(texture_data, 0, texture_width * texture_height * sizeof(uint8_t));

    for (auto [c, info] : glyphs) {
        for (uint32_t y = 0; y < info.bitmap_rows; ++y) {
            for (uint32_t x = 0; x < info.bitmap_width; ++x) {
                texture_data[(info.row + y) * texture_width + info.col + x] = info.buffer[y * info.bitmap_width + x];
            }
        }

        const glm::ivec2 size = glm::ivec2(info.bitmap_width, info.bitmap_rows);
        
        const Glyph glyph = {
            .size = size,
            .tex_size = glm::vec2(size) / texture_size,
            .bearing = glm::ivec2(info.bitmap_left, info.bitmap_top),
            .advance = info.advance.x,
            .texture_coords = glm::vec2(info.col, info.row) / texture_size
        };

        font.glyphs[c] = glyph;

        delete[] info.buffer;
    }

    font.texture = Renderer::CreateTexture(LLGL::TextureType::Texture2D, LLGL::ImageFormat::R, texture_width, texture_height, 1, TextureSampler::Linear, texture_data);

    font.font_size = FONT_SIZE;
    font.ascender = face->ascender >> 6;

    delete[] texture_data;

    FT_Done_Face(face);

    return true;
}