#include "assets.hpp"

#include <sstream>
#include <fstream>
#include <array>

#include <LLGL/Utils/VertexFormat.h>
#include <LLGL/Shader.h>
#include <LLGL/ShaderFlags.h>
#include <LLGL/VertexAttribute.h>
#include <STB/stb_image.h>
#include <ft2build.h>
#include <freetype/freetype.h>

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
        stages(stages),
        vertex_format_assets() {}

    explicit AssetShader(std::string file_name, uint8_t stages, VertexFormatAsset vertex_format) :
        file_name(std::move(file_name)),
        stages(stages),
        vertex_format_assets({vertex_format}) {}

    explicit AssetShader(std::string file_name, uint8_t stages, const std::vector<VertexFormatAsset> vertex_formats) :
        file_name(std::move(file_name)),
        stages(stages),
        vertex_format_assets(vertex_formats) {}
};

struct AssetComputeShader {
    std::string file_name;
    std::string func_name;

    explicit AssetComputeShader(std::string file_name, std::string func_name) :
        file_name(std::move(file_name)),
        func_name(std::move(func_name)) {}
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

    { TextureAsset::Background0, AssetTexture("assets/sprites/backgrounds/Background_0.png", TextureSampler::Nearest) },
    { TextureAsset::Background7, AssetTexture("assets/sprites/backgrounds/Background_7.png", TextureSampler::Nearest) },
    { TextureAsset::Background55, AssetTexture("assets/sprites/backgrounds/Background_55.png", TextureSampler::Nearest) },
    { TextureAsset::Background74, AssetTexture("assets/sprites/backgrounds/Background_74.png", TextureSampler::Nearest) },
    { TextureAsset::Background77, AssetTexture("assets/sprites/backgrounds/Background_77.png", TextureSampler::Nearest) },
    { TextureAsset::Background78, AssetTexture("assets/sprites/backgrounds/Background_78.png", TextureSampler::Nearest) },
    { TextureAsset::Background90, AssetTexture("assets/sprites/backgrounds/Background_90.png", TextureSampler::Nearest) },
    { TextureAsset::Background91, AssetTexture("assets/sprites/backgrounds/Background_91.png", TextureSampler::Nearest) },
    { TextureAsset::Background92, AssetTexture("assets/sprites/backgrounds/Background_92.png", TextureSampler::Nearest) },
    { TextureAsset::Background93, AssetTexture("assets/sprites/backgrounds/Background_93.png", TextureSampler::Nearest) },
    { TextureAsset::Background112, AssetTexture("assets/sprites/backgrounds/Background_112.png", TextureSampler::Nearest) },
    { TextureAsset::Background114, AssetTexture("assets/sprites/backgrounds/Background_114.png", TextureSampler::Nearest) },

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
    { TextureAsset::Particles,          AssetTextureAtlas(PARTICLES_ATLAS_COLUMNS, 12, glm::uvec2(8), glm::uvec2(2)) }
};

static const std::array BLOCK_ASSETS = std::to_array<std::pair<uint16_t, std::string>>({
    { static_cast<uint16_t>(BlockType::Dirt), "assets/sprites/tiles/Tiles_0.png" },
    { static_cast<uint16_t>(BlockType::Stone), "assets/sprites/tiles/Tiles_1.png" },
    { static_cast<uint16_t>(BlockType::Grass), "assets/sprites/tiles/Tiles_2.png" },
    { static_cast<uint16_t>(BlockType::Wood), "assets/sprites/tiles/Tiles_30.png" },
});

static const std::array WALL_ASSETS = std::to_array<std::pair<uint16_t, std::string>>({
    { static_cast<uint16_t>(WallType::DirtWall), "assets/sprites/walls/Wall_2.png" },
    { static_cast<uint16_t>(WallType::StoneWall), "assets/sprites/walls/Wall_1.png" },
});

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

static const std::array FONT_ASSETS = std::to_array<std::pair<FontAsset, std::string>>({
    { FontAsset::AndyBold, "assets/fonts/andy_bold.ttf" },
    { FontAsset::AndyRegular, "assets/fonts/andy_regular.otf" },
});

const std::pair<ShaderAsset, AssetShader> SHADER_ASSETS[] = {
    { ShaderAsset::BackgroundShader, AssetShader("background", ShaderStages::Vertex | ShaderStages::Fragment, VertexFormatAsset::BackgroundVertex) },
    { ShaderAsset::PostProcessShader,AssetShader("postprocess",ShaderStages::Vertex | ShaderStages::Fragment, VertexFormatAsset::PostProcessVertex) },
    { ShaderAsset::TilemapShader,    AssetShader("tilemap",    ShaderStages::Vertex | ShaderStages::Fragment, { VertexFormatAsset::TilemapVertex,  VertexFormatAsset::TilemapInstance  }) },
    { ShaderAsset::FontShader,       AssetShader("font",       ShaderStages::Vertex | ShaderStages::Fragment, { VertexFormatAsset::FontVertex,     VertexFormatAsset::FontInstance     }) },
    { ShaderAsset::SpriteShader,     AssetShader("sprite",     ShaderStages::Vertex | ShaderStages::Fragment, { VertexFormatAsset::SpriteVertex,   VertexFormatAsset::SpriteInstance   }) },
    { ShaderAsset::ParticleShader,   AssetShader("particle",   ShaderStages::Vertex | ShaderStages::Fragment, { VertexFormatAsset::ParticleVertex, VertexFormatAsset::ParticleInstance }) },
};

const std::pair<ComputeShaderAsset, AssetComputeShader> COMPUTE_SHADER_ASSETS[] = {
    { ComputeShaderAsset::ParticleComputeTransformShader, AssetComputeShader("particle", "CSComputeTransform") },
};

static struct AssetsState {
    std::unordered_map<uint16_t, Texture> items;
    std::unordered_map<TextureAsset, Texture> textures;
    std::unordered_map<TextureAsset, TextureAtlas> textures_atlases;
    std::unordered_map<ShaderAsset, ShaderPipeline> shaders;
    std::unordered_map<ComputeShaderAsset, LLGL::Shader*> compute_shaders;
    std::unordered_map<FontAsset, Font> fonts;
    std::unordered_map<VertexFormatAsset, LLGL::VertexFormat> vertex_formats;
    std::vector<LLGL::Sampler*> samplers;
    uint32_t texture_index = 0;
} state;

static Texture create_texture(uint32_t width, uint32_t height, uint32_t components, int sampler, const uint8_t* data, bool generate_mip_maps = false);
static Texture create_texture_array(uint32_t width, uint32_t height, uint32_t layers, uint32_t components, int sampler, const uint8_t* data, size_t data_size, bool generate_mip_maps = false);
static LLGL::Shader* load_shader(ShaderType shader_type, const std::string& name, const std::vector<ShaderDef>& shader_defs, const std::vector<LLGL::VertexAttribute>& vertex_attributes = {});
static LLGL::Shader* load_compute_shader(const std::string& name, const std::string& func_name, const std::vector<ShaderDef>& shader_defs);
static bool load_font(FT_Library ft, const std::string& path, Font& font);

template <size_t T>
static Texture load_texture_array(const std::array<std::pair<uint16_t, std::string>, T>& assets, int sampler, bool generate_mip_maps = false);

bool Assets::Load() {
    const uint8_t data[] = { 0xFF, 0xFF, 0xFF, 0xFF };
    state.textures[TextureAsset::Stub] = create_texture(1, 1, 4, TextureSampler::Nearest, data);

    for (const auto& [key, asset] : TEXTURE_ASSETS) {
        int width, height, components;

        uint8_t* data = stbi_load(asset.path.c_str(), &width, &height, &components, 4);
        if (data == nullptr) {
            LOG_ERROR("Couldn't load asset: %s", asset.path.c_str());
            return false;   
        }

        state.textures[key] = create_texture(width, height, components, asset.sampler, data);

        stbi_image_free(data);
    }

    for (const auto& [key, asset] : TEXTURE_ATLAS_ASSETS) {
        state.textures_atlases[key] = TextureAtlas::from_grid(Assets::GetTexture(key), asset.tile_size, asset.columns, asset.rows, asset.padding, asset.offset);
    }
    
    for (const auto& [key, asset] : ITEM_ASSETS) {
        int width, height, components;

        uint8_t* data = stbi_load(asset.c_str(), &width, &height, &components, 4);
        if (data == nullptr) {
            LOG_ERROR("Couldn't load asset: %s", asset.c_str());
            return false;   
        }

        state.items[key] = create_texture(width, height, components, TextureSampler::Nearest, data);

        stbi_image_free(data);
    }

    state.textures[TextureAsset::Tiles] = load_texture_array(BLOCK_ASSETS, TextureSampler::NearestMips, true);
    state.textures[TextureAsset::Walls] = load_texture_array(WALL_ASSETS, TextureSampler::Nearest);

    return true;
}

bool Assets::LoadShaders(const std::vector<ShaderDef>& shader_defs) {
    InitVertexFormats();

    for (const auto& [key, asset] : SHADER_ASSETS) {
        ShaderPipeline shader_pipeline;

        std::vector<LLGL::VertexAttribute> attributes;

        for (VertexFormatAsset asset : asset.vertex_format_assets) {
            const LLGL::VertexFormat& vertex_format = Assets::GetVertexFormat(asset);
            attributes.insert(attributes.end(), vertex_format.attributes.begin(), vertex_format.attributes.end());
        }

        if ((asset.stages & ShaderStages::Vertex) == ShaderStages::Vertex) {
            if (!(shader_pipeline.vs = load_shader(ShaderType::Vertex, asset.file_name, shader_defs, attributes)))
                return false;
        }
        
        if ((asset.stages & ShaderStages::Fragment) == ShaderStages::Fragment) {
            if (!(shader_pipeline.ps = load_shader(ShaderType::Fragment, asset.file_name, shader_defs)))
                return false;
        }

        if ((asset.stages & ShaderStages::Geometry) == ShaderStages::Geometry) {
            if (!(shader_pipeline.gs = load_shader(ShaderType::Geometry, asset.file_name, shader_defs)))
                return false;
        }

        state.shaders[key] = shader_pipeline;
    }

    for (const auto& [key, asset] : COMPUTE_SHADER_ASSETS) {
        if (!(state.compute_shaders[key] = load_compute_shader(asset.file_name, asset.func_name, shader_defs)))
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
        if (!FileExists(path.c_str())) {
            LOG_ERROR("Failed to find font '%s'",  path.c_str());
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
        sampler_desc.maxLOD = 1.0f;
        sampler_desc.mipMapEnabled = false;
        sampler_desc.maxAnisotropy = 1;

        state.samplers[TextureSampler::Linear] = Renderer::Context()->CreateSampler(sampler_desc);
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

        state.samplers[TextureSampler::LinearMips] = Renderer::Context()->CreateSampler(sampler_desc);
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
        sampler_desc.maxLOD = 1.0f;
        sampler_desc.mipMapEnabled = false;
        sampler_desc.maxAnisotropy = 1;

        state.samplers[TextureSampler::Nearest] = Renderer::Context()->CreateSampler(sampler_desc);
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

        state.samplers[TextureSampler::NearestMips] = Renderer::Context()->CreateSampler(sampler_desc);
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
    LLGL::VertexFormat particle_vertex_format;
    LLGL::VertexFormat particle_instance_format;
    LLGL::VertexFormat lightmap_vertex_format;

    if (backend.IsGLSL()) {
        sprite_vertex_format.AppendAttribute({ "a_position", LLGL::Format::RG32Float, 0, 0, sizeof(SpriteVertex), 0, 0 });
    } else if (backend.IsHLSL()) {
        sprite_vertex_format.AppendAttribute({ "Position",   LLGL::Format::RG32Float, 0, 0, sizeof(SpriteVertex), 0, 0 });
    } else {
        sprite_vertex_format.AppendAttribute({ "position",   LLGL::Format::RG32Float, 0, 0, sizeof(SpriteVertex), 0, 0 });
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
            {"a_tile_tex_size", LLGL::Format::RG32Float, 3, offsetof(ChunkVertex,tile_tex_size), sizeof(ChunkVertex), 0, 0},
            {"a_wall_padding",  LLGL::Format::RG32Float, 2, offsetof(ChunkVertex,wall_padding),  sizeof(ChunkVertex), 0, 0},
            {"a_tile_padding",  LLGL::Format::RG32Float, 4, offsetof(ChunkVertex,tile_padding),  sizeof(ChunkVertex), 0, 0}
        };
    } else if (backend.IsHLSL()) {
        tilemap_vertex_format.attributes = {
            {"Position"   , LLGL::Format::RG32Float, 0, 0, sizeof(ChunkVertex), 0, 0 },
            {"WallTexSize", LLGL::Format::RG32Float, 1, offsetof(ChunkVertex,wall_tex_size), sizeof(ChunkVertex), 0, 0},
            {"TileTexSize", LLGL::Format::RG32Float, 3, offsetof(ChunkVertex,tile_tex_size), sizeof(ChunkVertex), 0, 0},
            {"WallPadding", LLGL::Format::RG32Float, 2, offsetof(ChunkVertex,wall_padding), sizeof(ChunkVertex), 0, 0},
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
            {"i_position",  LLGL::Format::RG32Float,   5, offsetof(ChunkInstance,position),  sizeof(ChunkInstance), 1, 1},
            {"i_atlas_pos", LLGL::Format::RG32Float,   6, offsetof(ChunkInstance,atlas_pos), sizeof(ChunkInstance), 1, 1},
            {"i_world_pos", LLGL::Format::RG32Float,   7, offsetof(ChunkInstance,world_pos), sizeof(ChunkInstance), 1, 1},
            {"i_tile_data",   LLGL::Format::R32UInt,   8, offsetof(ChunkInstance,tile_data), sizeof(ChunkInstance), 1, 1},
        };
    } else if (backend.IsHLSL()) {
        tilemap_instance_format.attributes = {
            {"I_Position",  LLGL::Format::RG32Float,   5, offsetof(ChunkInstance,position),  sizeof(ChunkInstance), 1, 1},
            {"I_AtlasPos",  LLGL::Format::RG32Float,   6, offsetof(ChunkInstance,atlas_pos), sizeof(ChunkInstance), 1, 1},
            {"I_WorldPos",  LLGL::Format::RG32Float,   7, offsetof(ChunkInstance,world_pos), sizeof(ChunkInstance), 1, 1},
            {"I_TileData",  LLGL::Format::R32UInt,     8, offsetof(ChunkInstance,tile_data), sizeof(ChunkInstance), 1, 1},
        };
    } else {
        tilemap_instance_format.attributes = {
            {"i_position",   LLGL::Format::RG32Float,   5, offsetof(ChunkInstance,position),  sizeof(ChunkInstance), 1, 1},
            {"i_atlas_pos",  LLGL::Format::RG32Float,   6, offsetof(ChunkInstance,atlas_pos), sizeof(ChunkInstance), 1, 1},
            {"i_world_pos",  LLGL::Format::RG32Float,   7, offsetof(ChunkInstance,world_pos), sizeof(ChunkInstance), 1, 1},
            {"i_tile_data",  LLGL::Format::R32UInt,     8, offsetof(ChunkInstance,tile_data), sizeof(ChunkInstance), 1, 1},
        };
    }

    if (backend.IsGLSL()) {
        font_vertex_format.AppendAttribute({"a_position", LLGL::Format::RG32Float, 0, 0, sizeof(GlyphVertex), 0, 0});
    } else if (backend.IsHLSL()) {
        font_vertex_format.AppendAttribute({"Position",   LLGL::Format::RG32Float, 0, 0, sizeof(GlyphVertex), 0, 0});
    } else {
        font_vertex_format.AppendAttribute({"position",   LLGL::Format::RG32Float, 0, 0, sizeof(GlyphVertex), 0, 0});
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
        background_vertex_format.AppendAttribute({"a_position", LLGL::Format::RG32Float});
        background_vertex_format.AppendAttribute({"a_uv", LLGL::Format::RG32Float});
        background_vertex_format.AppendAttribute({"a_size", LLGL::Format::RG32Float});
        background_vertex_format.AppendAttribute({"a_tex_size", LLGL::Format::RG32Float});
        background_vertex_format.AppendAttribute({"a_speed", LLGL::Format::RG32Float});
        background_vertex_format.AppendAttribute({"a_nonscale", LLGL::Format::R32SInt});
    } else if (backend.IsHLSL()) {
        background_vertex_format.AppendAttribute({"Position", LLGL::Format::RG32Float});
        background_vertex_format.AppendAttribute({"Uv", LLGL::Format::RG32Float});
        background_vertex_format.AppendAttribute({"Size", LLGL::Format::RG32Float});
        background_vertex_format.AppendAttribute({"TexSize", LLGL::Format::RG32Float});
        background_vertex_format.AppendAttribute({"Speed", LLGL::Format::RG32Float});
        background_vertex_format.AppendAttribute({"NonScale", LLGL::Format::R32SInt});
    } else {
        background_vertex_format.AppendAttribute({"position", LLGL::Format::RG32Float});
        background_vertex_format.AppendAttribute({"uv", LLGL::Format::RG32Float});
        background_vertex_format.AppendAttribute({"size", LLGL::Format::RG32Float});
        background_vertex_format.AppendAttribute({"tex_size", LLGL::Format::RG32Float});
        background_vertex_format.AppendAttribute({"speed", LLGL::Format::RG32Float});
        background_vertex_format.AppendAttribute({"nonscale", LLGL::Format::R32SInt});
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
    state.vertex_formats[VertexFormatAsset::ParticleVertex] = particle_vertex_format;
    state.vertex_formats[VertexFormatAsset::ParticleInstance] = particle_instance_format;
    state.vertex_formats[VertexFormatAsset::PostProcessVertex] = lightmap_vertex_format;
}

void Assets::DestroyTextures() {
    for (auto& entry : state.textures) {
        Renderer::Context()->Release(*entry.second.texture);
    }
}

void Assets::DestroySamplers() {
    for (auto& sampler : state.samplers) {
        Renderer::Context()->Release(*sampler);
    }
}

void Assets::DestroyShaders() {
    for (auto& entry : state.shaders) {
        entry.second.Unload(Renderer::Context());
    }
}

const Texture& Assets::GetTexture(TextureAsset key) {
    ASSERT(state.textures.contains(key), "Key not found");
    return state.textures.find(key)->second;
}

const TextureAtlas& Assets::GetTextureAtlas(TextureAsset key) {
    ASSERT(state.textures_atlases.contains(key), "Key not found");
    return state.textures_atlases.find(key)->second;
}

const Font& Assets::GetFont(FontAsset key) {
    ASSERT(state.fonts.contains(key), "Key not found");
    return state.fonts.find(key)->second;
}

const Texture& Assets::GetItemTexture(size_t index) {
    ASSERT(state.items.contains(index), "Key not found");
    return state.items[index];
}

const ShaderPipeline& Assets::GetShader(ShaderAsset key) {
    ASSERT(state.shaders.contains(key), "Key not found");
    return state.shaders.find(key)->second;
}

LLGL::Shader* Assets::GetComputeShader(ComputeShaderAsset key) {
    ASSERT(state.compute_shaders.contains(key), "Key not found");
    return state.compute_shaders.find(key)->second;
}

LLGL::Sampler& Assets::GetSampler(size_t index) {
    ASSERT(index < state.samplers.size(), "Index is out of bounds");
    return *state.samplers[index];
}

const LLGL::VertexFormat& Assets::GetVertexFormat(VertexFormatAsset key) {
    ASSERT(state.vertex_formats.contains(key), "Key not found");
    return state.vertex_formats.find(key)->second;
}

Texture create_texture(uint32_t width, uint32_t height, uint32_t components, int sampler, const uint8_t* data, bool generate_mip_maps) {
    LLGL::TextureDescriptor texture_desc;
    texture_desc.extent = LLGL::Extent3D(width, height, 1);
    texture_desc.bindFlags = LLGL::BindFlags::Sampled | LLGL::BindFlags::ColorAttachment;
    texture_desc.cpuAccessFlags = 0;
    texture_desc.miscFlags = LLGL::MiscFlags::GenerateMips * generate_mip_maps;

    LLGL::ImageView image_view;
    if (components == 4) {
        image_view.format = LLGL::ImageFormat::RGBA;
    } else if (components == 3) {
        image_view.format = LLGL::ImageFormat::RGB;
    } else if (components == 2) {
        image_view.format = LLGL::ImageFormat::RG;
    } else if (components == 1) {
        image_view.format = LLGL::ImageFormat::R;
    }
    image_view.dataType = LLGL::DataType::UInt8;
    image_view.data = data;
    image_view.dataSize = width * height * components;

    Texture texture;
    texture.id = state.texture_index++;
    texture.texture = Renderer::Context()->CreateTexture(texture_desc, &image_view);
    texture.sampler = sampler;
    texture.size = glm::uvec2(width, height);

    return texture;
}

Texture create_texture_array(uint32_t width, uint32_t height, uint32_t layers, uint32_t components, int sampler, const uint8_t* data, size_t data_size, bool generate_mip_maps) {
    LLGL::TextureDescriptor texture_desc;
    texture_desc.type = LLGL::TextureType::Texture2DArray;
    texture_desc.extent = LLGL::Extent3D(width, height, 1);
    texture_desc.arrayLayers = layers;
    texture_desc.bindFlags = LLGL::BindFlags::Sampled | LLGL::BindFlags::ColorAttachment;
    texture_desc.cpuAccessFlags = 0;
    texture_desc.miscFlags = LLGL::MiscFlags::GenerateMips * generate_mip_maps;

    LLGL::ImageView image_view;
    image_view.format = (components == 4 ? LLGL::ImageFormat::RGBA : LLGL::ImageFormat::RGB);
    image_view.dataType = LLGL::DataType::UInt8;
    image_view.data = data;
    image_view.dataSize = data_size;

    Texture texture;
    texture.id = state.texture_index++;
    texture.texture = Renderer::Context()->CreateTexture(texture_desc, &image_view);
    texture.sampler = sampler;
    texture.size = glm::uvec2(width, height);

    return texture;
}

template <size_t T>
Texture load_texture_array(const std::array<std::pair<uint16_t, std::string>, T>& assets, int sampler, bool generate_mip_maps) {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t layers_count = 0;

    struct Layer {
        uint8_t* data;
        size_t size;
    };

    std::unordered_map<uint32_t, Layer> layer_to_data_map;
    layer_to_data_map.reserve(assets.size());

    for (const std::pair<uint16_t, std::string>& asset : assets) {
        layers_count = glm::max(layers_count, static_cast<uint32_t>(asset.first));

        int w, h;
        uint8_t* layer_data = stbi_load(asset.second.c_str(), &w, &h, nullptr, 4);

        layer_to_data_map[asset.first] = Layer {
            .data = layer_data,
            .size = static_cast<size_t>(w * h * 4)
        };

        width = glm::max(width, static_cast<uint32_t>(w));
        height = glm::max(height, static_cast<uint32_t>(h));
    }
    layers_count += 1;

    const size_t data_size = width * height * layers_count * 4;
    uint8_t* image_data = new uint8_t[data_size];

    for (const auto& [layer, layer_data] : layer_to_data_map) {
        memcpy(&image_data[width * height * 4 * layer], layer_data.data, layer_data.size);
        stbi_image_free(layer_data.data);
    }
    
    return create_texture_array(width, height, layers_count, 4, sampler, image_data, data_size, generate_mip_maps);
}

LLGL::Shader* load_shader(
    ShaderType shader_type,
    const std::string& name,
    const std::vector<ShaderDef>& shader_defs,
    const std::vector<LLGL::VertexAttribute>& vertex_attributes
) {
    const RenderBackend backend = Renderer::Backend();

    const std::string path = backend.AssetFolder() + name + shader_type.FileExtension(backend);

    if (!FileExists(path.c_str())) {
        LOG_ERROR("Failed to find shader '%s'", path.c_str());
        return nullptr;
    }

    std::ifstream shader_file;
    shader_file.open(path);

    std::stringstream shader_source_str;
    shader_source_str << shader_file.rdbuf();

    std::string shader_source = shader_source_str.str();

    for (const ShaderDef& shader_def : shader_defs) {
        size_t pos;
        while ((pos = shader_source.find(shader_def.name)) != std::string::npos) {
            shader_source.replace(pos, shader_def.name.length(), shader_def.value);
        }
    }

    shader_file.close();

    LLGL::ShaderDescriptor shader_desc;
    shader_desc.type = shader_type.ToLLGLType();
    shader_desc.sourceType = LLGL::ShaderSourceType::CodeString;

    if (shader_type.IsVertex()) {
        shader_desc.vertex.inputAttribs = vertex_attributes;
    }

    if (backend.IsVulkan()) {
        shader_desc.source = path.c_str();
        shader_desc.sourceType = LLGL::ShaderSourceType::BinaryFile;
    } else {
        shader_desc.entryPoint = shader_type.EntryPoint(backend);
        shader_desc.source = shader_source.c_str();
        shader_desc.sourceSize = shader_source.length();
        shader_desc.profile = shader_type.Profile(backend);
    }

#if DEBUG
    shader_desc.flags |= LLGL::ShaderCompileFlags::NoOptimization;
#else
    shader_desc.flags |= LLGL::ShaderCompileFlags::OptimizationLevel3;
#endif

    LLGL::Shader* shader = Renderer::Context()->CreateShader(shader_desc);
    if (const LLGL::Report* report = shader->GetReport()) {
        if (*report->GetText() != '\0') {
            if (report->HasErrors()) {
                LOG_ERROR("Failed to create a shader. File: %s\nError: %s", path.c_str(), report->GetText());
                return nullptr;
            }
            
            LOG_INFO("%s", report->GetText());
        }
    }

    return shader;
}

LLGL::Shader* load_compute_shader(const std::string& name, const std::string& func_name, const std::vector<ShaderDef>& shader_defs) {
    const RenderBackend backend = Renderer::Backend();
    const ShaderType shader_type = ShaderType::Compute;

    const std::string path = backend.AssetFolder() + name + shader_type.FileExtension(backend);

    if (!FileExists(path.c_str())) {
        LOG_ERROR("Failed to find shader '%s'", path.c_str());
        return nullptr;
    }

    std::ifstream shader_file;
    shader_file.open(path);

    std::stringstream shader_source_str;
    shader_source_str << shader_file.rdbuf();

    std::string shader_source = shader_source_str.str();

    for (const ShaderDef& shader_def : shader_defs) {
        size_t pos;
        while ((pos = shader_source.find(shader_def.name)) != std::string::npos) {
            shader_source.replace(pos, shader_def.name.length(), shader_def.value);
        }
    }

    shader_file.close();

    LLGL::ShaderDescriptor shader_desc;
    shader_desc.type = shader_type.ToLLGLType();
    shader_desc.sourceType = LLGL::ShaderSourceType::CodeString;

    if (backend.IsVulkan()) {
        shader_desc.source = path.c_str();
        shader_desc.sourceType = LLGL::ShaderSourceType::BinaryFile;
    } else {
        if (!backend.IsOpenGL()) {
            shader_desc.entryPoint = func_name.c_str();
        }
        
        shader_desc.source = shader_source.c_str();
        shader_desc.sourceSize = shader_source.length();
        shader_desc.profile = shader_type.Profile(backend);
    }

#if DEBUG
    shader_desc.flags |= LLGL::ShaderCompileFlags::NoOptimization;
#else
    shader_desc.flags |= LLGL::ShaderCompileFlags::OptimizationLevel3;
#endif

    LLGL::Shader* shader = Renderer::Context()->CreateShader(shader_desc);
    if (const LLGL::Report* report = shader->GetReport()) {
        if (*report->GetText() != '\0') {
            if (report->HasErrors()) {
                LOG_ERROR("Failed to create a shader. File: %s\nError: %s", path.c_str(), report->GetText());
                return nullptr;
            }
            
            LOG_INFO("%s", report->GetText());
        }
    }

    return shader;
}

bool load_font(FT_Library ft, const std::string& path, Font& font) {
    FT_Face face;
    if (FT_New_Face(ft, path.c_str(), 0, &face)) {
        LOG_ERROR("Failed to load font: %s", path.c_str());
        return false;
    }

    constexpr uint32_t FONT_SIZE = 22;
    constexpr uint32_t PADDING = 2;

    FT_Set_Pixel_Sizes(face, 0, FONT_SIZE);

    constexpr uint32_t texture_width = 512;
    constexpr uint32_t texture_height = 512;

    const glm::vec2 texture_size = glm::vec2(texture_width, texture_height);

    uint32_t row = 0;
    uint32_t col = PADDING;

    uint8_t* texture_data = new uint8_t[texture_width * texture_height];
    memset(texture_data, 0, texture_width * texture_height * sizeof(uint8_t));

    for (unsigned char c = 32; c < 127; ++c) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            LOG_ERROR("Failed to load glyph '%c'", c);
            continue;
        }

        if (col + face->glyph->bitmap.width + PADDING >= texture_width) {
            col = PADDING;
            row += FONT_SIZE;
        }

        for (uint32_t y = 0; y < face->glyph->bitmap.rows; ++y) {
            for (uint32_t x = 0; x < face->glyph->bitmap.width; ++x) {
                texture_data[(row + y) * texture_width + col + x] = face->glyph->bitmap.buffer[y * face->glyph->bitmap.width + x];
            }
        }

        const glm::ivec2 size = glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows);
        
        Glyph glyph = {
            .size = size,
            .tex_size = glm::vec2(size) / texture_size,
            .bearing = glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            .advance = face->glyph->advance.x,
            .texture_coords = glm::vec2(col, row) / texture_size
        };

        font.glyphs[c] = glyph;

        col += face->glyph->bitmap.width + PADDING;
    }

    font.texture = create_texture(texture_width, texture_height, 1, TextureSampler::Linear, texture_data);
    font.font_size = FONT_SIZE;

    FT_Done_Face(face);

    return true;
}