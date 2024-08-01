#include <sstream>
#include <fstream>
#include <array>

#include <LLGL/Utils/VertexFormat.h>
#include <LLGL/Shader.h>
#include <LLGL/ShaderFlags.h>
#include <LLGL/VertexAttribute.h>
#include <STB/stb_image.h>

#include "assets.hpp"
#include "renderer/assets.hpp"
#include "renderer/renderer.hpp"
#include "log.hpp"
#include "utils.hpp"
#include "types/shader_pipeline.hpp"
#include "types/shader_type.hpp"
#include "vulkan_shader_compiler.hpp"

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

namespace ShaderStages {
    enum : uint8_t {
        Vertex = 1 << 0,
        Fragment = 1 << 1,
        Geometry = 1 << 2
    };
}

struct AssetShader {
    std::string name;
    uint8_t stages;
    std::vector<LLGL::VertexAttribute> vertex_attributes;

    explicit AssetShader(std::string name, uint8_t stages) :
        name(std::move(name)),
        stages(stages),
        vertex_attributes() {}

    explicit AssetShader(std::string name, uint8_t stages, const LLGL::VertexFormat& vertex_format) :
        name(std::move(name)),
        stages(stages),
        vertex_attributes(vertex_format.attributes) {}
};

static const std::pair<AssetKey, AssetTexture> TEXTURE_ASSETS[] = {
    { AssetKey::TexturePlayerHair,         AssetTexture("assets/sprites/player/Player_Hair_1.png") },
    { AssetKey::TexturePlayerHead,         AssetTexture("assets/sprites/player/Player_0_0.png") },
    { AssetKey::TexturePlayerChest,        AssetTexture("assets/sprites/player/Player_Body.png") },
    { AssetKey::TexturePlayerLegs,         AssetTexture("assets/sprites/player/Player_0_11.png") },
    { AssetKey::TexturePlayerLeftHand,     AssetTexture("assets/sprites/player/Player_Left_Hand.png") },
    { AssetKey::TexturePlayerLeftShoulder, AssetTexture("assets/sprites/player/Player_Left_Shoulder.png") },
    { AssetKey::TexturePlayerRightArm,     AssetTexture("assets/sprites/player/Player_Right_Arm.png") },
    { AssetKey::TexturePlayerLeftEye,      AssetTexture("assets/sprites/player/Player_0_1.png") },
    { AssetKey::TexturePlayerRightEye,     AssetTexture("assets/sprites/player/Player_0_2.png") },

    { AssetKey::TextureUiCursorForeground,    AssetTexture("assets/sprites/ui/Cursor_0.png", TextureSampler::Linear) },
    { AssetKey::TextureUiCursorBackground,    AssetTexture("assets/sprites/ui/Cursor_11.png", TextureSampler::Linear) },
    { AssetKey::TextureUiInventoryBackground, AssetTexture("assets/sprites/ui/Inventory_Back.png", TextureSampler::Nearest) },
    { AssetKey::TextureUiInventorySelected,   AssetTexture("assets/sprites/ui/Inventory_Back14.png", TextureSampler::Nearest) },
    { AssetKey::TextureUiInventoryHotbar,     AssetTexture("assets/sprites/ui/Inventory_Back9.png", TextureSampler::Nearest) },

    { AssetKey::TextureParticles, AssetTexture("assets/sprites/Particles.png") }
};

static const std::pair<AssetKey, AssetTextureAtlas> TEXTURE_ATLAS_ASSETS[] = {
    { AssetKey::TexturePlayerHair,         AssetTextureAtlas(1, 14, glm::uvec2(40, 64)) },
    { AssetKey::TexturePlayerHead,         AssetTextureAtlas(1, 14, glm::uvec2(40, 48)) },
    { AssetKey::TexturePlayerChest,        AssetTextureAtlas(1, 14, glm::uvec2(32, 64), glm::uvec2(8, 0)) },
    { AssetKey::TexturePlayerLegs,         AssetTextureAtlas(1, 19, glm::uvec2(40, 64)) },
    { AssetKey::TexturePlayerLeftHand,     AssetTextureAtlas(27, 1, glm::uvec2(32, 64)) },
    { AssetKey::TexturePlayerLeftShoulder, AssetTextureAtlas(27, 1, glm::uvec2(32, 64)) },
    { AssetKey::TexturePlayerRightArm,     AssetTextureAtlas(18, 1, glm::uvec2(32, 80)) },
    { AssetKey::TexturePlayerLeftEye,      AssetTextureAtlas(1, 20, glm::uvec2(40, 64)) },
    { AssetKey::TexturePlayerRightEye,     AssetTextureAtlas(1, 20, glm::uvec2(40, 64)) },
    { AssetKey::TextureParticles,          AssetTextureAtlas(PARTICLES_ATLAS_COLUMNS, 12, glm::uvec2(8), glm::uvec2(2)) }
};

static const std::pair<uint16_t, std::string> BLOCK_ASSETS[] = {
    { static_cast<uint16_t>(BlockType::Dirt), "assets/sprites/tiles/Tiles_0.png" },
    { static_cast<uint16_t>(BlockType::Stone), "assets/sprites/tiles/Tiles_1.png" },
    { static_cast<uint16_t>(BlockType::Grass), "assets/sprites/tiles/Tiles_2.png" },
    { static_cast<uint16_t>(BlockType::Wood), "assets/sprites/tiles/Tiles_30.png" },
};

static const std::pair<uint16_t, std::string> WALL_ASSETS[] = {
    { static_cast<uint16_t>(WallType::DirtWall), "assets/sprites/walls/Wall_2.png" },
    { static_cast<uint16_t>(WallType::StoneWall), "assets/sprites/walls/Wall_1.png" },
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

static const std::pair<FontKey, std::string> FONT_ASSETS[] = {
    { FontKey::AndyBold, "assets/fonts/andy_bold.ttf" },
    { FontKey::AndyRegular, "assets/fonts/andy_regular.otf" },
};

static struct AssetsState {
    std::unordered_map<uint16_t, Texture> items;
    std::unordered_map<AssetKey, Texture> textures;
    std::unordered_map<AssetKey, TextureAtlas> textures_atlases;
    std::unordered_map<ShaderAssetKey, ShaderPipeline> shaders;
    std::vector<LLGL::Sampler*> samplers;
    // std::unordered_map<FontKey, Font> fonts;
    uint32_t texture_index = 0;
} state;

Texture create_texture(uint32_t width, uint32_t height, uint32_t components, int sampler, const uint8_t* data);
Texture create_texture_array_empty(uint32_t width, uint32_t height, uint32_t layers, uint32_t components, int sampler);
inline LLGL::Shader* load_shader(ShaderType shader_type, const std::string& name, const std::vector<ShaderDef>& shader_defs, const std::vector<LLGL::VertexAttribute>& vertex_attributes = {});

bool Assets::Load() {
    using Constants::MAX_TILE_TEXTURE_WIDTH;
    using Constants::MAX_TILE_TEXTURE_HEIGHT;
    using Constants::MAX_WALL_TEXTURE_WIDTH;
    using Constants::MAX_WALL_TEXTURE_HEIGHT;

    const uint8_t data[] = { 0xFF, 0xFF, 0xFF, 0xFF };
    state.textures[AssetKey::TextureStub] = create_texture(1, 1, 4, TextureSampler::Nearest, data);

    for (const auto& asset : TEXTURE_ASSETS) {
        int width, height, components;

        uint8_t* data = stbi_load(asset.second.path.c_str(), &width, &height, &components, 0);
        if (data == nullptr) {
            LOG_ERROR("Couldn't load asset: %s", asset.second.path.c_str());
            return false;   
        }

        state.textures[asset.first] = create_texture(width, height, components, asset.second.sampler, data);

        stbi_image_free(data);
    }

    for (const auto& asset : TEXTURE_ATLAS_ASSETS) {
        state.textures_atlases[asset.first] = TextureAtlas::from_grid(Assets::GetTexture(asset.first), asset.second.tile_size, asset.second.columns, asset.second.rows, asset.second.padding, asset.second.offset);
    }
    
    for (const auto& asset : ITEM_ASSETS) {
        int width, height, components;

        uint8_t* data = stbi_load(asset.second.c_str(), &width, &height, &components, 0);
        if (data == nullptr) {
            LOG_ERROR("Couldn't load asset: %s", asset.second.c_str());
            return false;   
        }

        state.items[asset.first] = create_texture(width, height, components, TextureSampler::Nearest, data);

        stbi_image_free(data);
    }

    uint16_t block_tex_count = 0;
    for (const auto& asset : BLOCK_ASSETS) {
        block_tex_count = glm::max(block_tex_count, asset.first);
    }

    uint16_t wall_tex_count = 0;
    for (const auto& asset : WALL_ASSETS) {
        wall_tex_count = glm::max(wall_tex_count, asset.first);
    }
    
    const Texture tiles = create_texture_array_empty(MAX_TILE_TEXTURE_WIDTH, MAX_TILE_TEXTURE_HEIGHT, block_tex_count + 1, 4, TextureSampler::Nearest);
    const Texture walls = create_texture_array_empty(MAX_WALL_TEXTURE_WIDTH, MAX_WALL_TEXTURE_HEIGHT, wall_tex_count + 1, 4, TextureSampler::Nearest);

    for (const auto& asset : BLOCK_ASSETS) {
        int width, height, components;
        uint8_t* data = stbi_load(asset.second.c_str(), &width, &height, &components, 0);

        LLGL::ImageView image_view;
        image_view.format = (components == 4 ? LLGL::ImageFormat::RGBA : LLGL::ImageFormat::RGB);
        image_view.dataType = LLGL::DataType::UInt8;
        image_view.data = data;
        image_view.dataSize = width * height * components;

        Renderer::Context()->WriteTexture(
            *tiles.texture,
            LLGL::TextureRegion(LLGL::TextureSubresource(asset.first, 0), LLGL::Offset3D(), LLGL::Extent3D(width, height, 1)),
            image_view
        );

        stbi_image_free(data);
    }

    for (const auto& asset : WALL_ASSETS) {
        int width, height, components;
        uint8_t* data = stbi_load(asset.second.c_str(), &width, &height, &components, 0);

        LLGL::ImageView image_view;
        image_view.format = (components == 4 ? LLGL::ImageFormat::RGBA : LLGL::ImageFormat::RGB);
        image_view.dataType = LLGL::DataType::UInt8;
        image_view.data = data;
        image_view.dataSize = width * height * components;

        Renderer::Context()->WriteTexture(
            *walls.texture,
            LLGL::TextureRegion(LLGL::TextureSubresource(asset.first, 0), LLGL::Offset3D(), LLGL::Extent3D(width, height, 1)),
            image_view
        );

        stbi_image_free(data);
    }

    state.textures[AssetKey::TextureTiles] = tiles;
    state.textures[AssetKey::TextureWalls] = walls;

    return true;
}

bool Assets::LoadShaders(const std::vector<ShaderDef>& shader_defs) {
    const std::pair<ShaderAssetKey, AssetShader> SHADER_ASSETS[] = {
        { ShaderAssetKey::SpriteShader, AssetShader("sprite", ShaderStages::Vertex | ShaderStages::Fragment, SpriteVertexFormat()) },
        { ShaderAssetKey::TilemapShader, AssetShader("tilemap", ShaderStages::Vertex | ShaderStages::Fragment | ShaderStages::Geometry, TilemapVertexFormat()) },
    };

    for (const auto& [key, asset] : SHADER_ASSETS) {
        ShaderPipeline shader_pipeline;

        if ((asset.stages & ShaderStages::Vertex) == ShaderStages::Vertex) {
            if (!(shader_pipeline.vs = load_shader(ShaderType::Vertex, asset.name, shader_defs, asset.vertex_attributes)))
                return false;
        }
        
        if ((asset.stages & ShaderStages::Fragment) == ShaderStages::Fragment) {
            if (!(shader_pipeline.ps = load_shader(ShaderType::Fragment, asset.name, shader_defs)))
                return false;
        }

        if ((asset.stages & ShaderStages::Geometry) == ShaderStages::Geometry) {
            if (!(shader_pipeline.gs = load_shader(ShaderType::Geometry, asset.name, shader_defs)))
                return false;
        }

        state.shaders[key] = shader_pipeline;
    }

    return true;
};

bool Assets::InitSamplers() {
    state.samplers.resize(2);
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
        sampler_desc.maxAnisotropy = 1;

        state.samplers[TextureSampler::Linear] = Renderer::Context()->CreateSampler(sampler_desc);
    }

    {
        LLGL::SamplerDescriptor sampler_desc;
        sampler_desc.addressModeU = LLGL::SamplerAddressMode::Clamp;
        sampler_desc.addressModeV = LLGL::SamplerAddressMode::Clamp;
        sampler_desc.addressModeW = LLGL::SamplerAddressMode::Clamp;
        sampler_desc.magFilter = LLGL::SamplerFilter::Nearest;
        sampler_desc.minFilter = LLGL::SamplerFilter::Nearest;
        sampler_desc.mipMapFilter = LLGL::SamplerFilter::Nearest;
        sampler_desc.minLOD = 0.0f;
        sampler_desc.maxLOD = 1.0f;
        sampler_desc.maxAnisotropy = 1;

        state.samplers[TextureSampler::Nearest] = Renderer::Context()->CreateSampler(sampler_desc);
    }

    return true;
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

const Texture& Assets::GetTexture(AssetKey key) {
    ASSERT(state.textures.contains(key), "Key not found");
    return state.textures[key];
}

const TextureAtlas& Assets::GetTextureAtlas(AssetKey key) {
    ASSERT(state.textures_atlases.contains(key), "Key not found");
    return state.textures_atlases[key];
}

const Texture& Assets::GetItemTexture(size_t index) {
    ASSERT(state.items.contains(index), "Key not found");
    return state.items[index];
}

const ShaderPipeline& Assets::GetShader(ShaderAssetKey key) {
    ASSERT(state.shaders.contains(key), "Key not found");
    return state.shaders[key];
}

LLGL::Sampler& Assets::GetSampler(size_t index) {
    ASSERT(index < state.samplers.size(), "Index is out of bounds");
    return *state.samplers[index];
}

Texture create_texture(uint32_t width, uint32_t height, uint32_t components, int sampler, const uint8_t* data) {
    LLGL::TextureDescriptor texture_desc;
    texture_desc.extent = LLGL::Extent3D(width, height, 1);
    texture_desc.bindFlags = LLGL::BindFlags::Sampled;
    texture_desc.cpuAccessFlags = 0;
    texture_desc.miscFlags = 0;

    LLGL::ImageView image_view;
    image_view.format = (components == 4 ? LLGL::ImageFormat::RGBA : LLGL::ImageFormat::RGB);
    image_view.dataType = LLGL::DataType::UInt8;
    image_view.data = data;
    image_view.dataSize = width * height *components;

    Texture texture;
    texture.id = state.texture_index++;
    texture.texture = Renderer::Context()->CreateTexture(texture_desc, &image_view);
    texture.sampler = sampler;

    return texture;
}

Texture create_texture_array_empty(uint32_t width, uint32_t height, uint32_t layers, uint32_t components, int sampler) {
    LLGL::TextureDescriptor texture_desc;
    texture_desc.type = LLGL::TextureType::Texture2DArray;
    texture_desc.extent = LLGL::Extent3D(width, height, 1);
    texture_desc.arrayLayers = layers;
    texture_desc.bindFlags = LLGL::BindFlags::Sampled;
    texture_desc.cpuAccessFlags = 0;
    texture_desc.miscFlags = 0;

    Texture texture;
    texture.id = state.texture_index++;
    texture.texture = Renderer::Context()->CreateTexture(texture_desc);
    texture.sampler = sampler;

    return texture;
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

    const std::string spirv_path = path + ".spv";
    if (backend.IsVulkan()) {
        if (!FileExists(spirv_path.c_str())) {
            if (!CompileVulkanShader(shader_type, shader_source.c_str(), shader_source.length(), spirv_path.c_str())) {
                return nullptr;
            }
        }

        shader_desc.source = spirv_path.c_str();
        shader_desc.sourceType = LLGL::ShaderSourceType::BinaryFile;
    } else {
        shader_desc.source = shader_source.c_str();
        shader_desc.sourceSize = shader_source.length();
        shader_desc.entryPoint = shader_type.EntryPoint(backend);
        shader_desc.profile = shader_type.Profile(backend);
        if (backend.IsMetal()) {
            shader_desc.flags |= LLGL::ShaderCompileFlags::DefaultLibrary;
        }
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