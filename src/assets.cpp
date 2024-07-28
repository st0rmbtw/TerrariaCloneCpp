#include <sstream>
#include <fstream>

#include <LLGL/Utils/VertexFormat.h>
#include <STB/stb_image.h>
#include "assets.hpp"
#include "renderer/assets.hpp"
#include "renderer/renderer.hpp"
#include "log.hpp"
#include "utils.hpp"
#include "types/shader_pipeline.hpp"

#ifdef BACKEND_VULKAN
#include "vulkan_shader_compiler.hpp"
#endif

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
    uint32_t texture_index;
} state;

Texture create_texture(uint32_t width, uint32_t height, uint32_t layers, uint32_t components, int sampler, uint8_t* data);
Texture create_texture_array_empty(uint32_t width, uint32_t height, uint32_t layers, uint32_t components, int sampler);
inline LLGL::Shader* load_shader(LLGL::ShaderType shader_type, const std::string& name, const LLGL::VertexFormat* vertex_format, const std::vector<ShaderDef>& shader_defs);

bool Assets::Load() {
    using Constants::MAX_TILE_TEXTURE_WIDTH;
    using Constants::MAX_TILE_TEXTURE_HEIGHT;
    using Constants::MAX_WALL_TEXTURE_WIDTH;
    using Constants::MAX_WALL_TEXTURE_HEIGHT;

    uint8_t data[] = { 0xFF, 0xFF, 0xFF, 0xFF };
    state.textures[AssetKey::TextureStub] = create_texture(1, 1, 1, 4, TextureSampler::Nearest, data);

    for (auto& asset : TEXTURE_ASSETS) {
        int width, height, components;

        uint8_t* data = stbi_load(asset.second.path.c_str(), &width, &height, &components, 0);
        if (data == nullptr) {
            LOG_ERROR("Couldn't load asset: %s", asset.second.path.c_str());
            return false;   
        }

        state.textures[asset.first] = create_texture(width, height, 1, components, asset.second.sampler, data);

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

        state.items[asset.first] = create_texture(width, height, 1, components, TextureSampler::Nearest, data);

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
    
    Texture tiles = create_texture_array_empty(MAX_TILE_TEXTURE_WIDTH, MAX_TILE_TEXTURE_HEIGHT, block_tex_count + 1, 4, TextureSampler::Nearest);
    Texture walls = create_texture_array_empty(MAX_WALL_TEXTURE_WIDTH, MAX_WALL_TEXTURE_HEIGHT, wall_tex_count + 1, 4, TextureSampler::Nearest);

    for (const auto& asset : BLOCK_ASSETS) {
        int width, height, components;
        uint8_t* data = stbi_load(asset.second.c_str(), &width, &height, &components, 0);

        LLGL::ImageView image_view;
        image_view.format = (components == 4 ? LLGL::ImageFormat::RGBA : LLGL::ImageFormat::RGB);
        image_view.dataType = LLGL::DataType::UInt8;
        image_view.data = data;
        image_view.dataSize = static_cast<std::size_t>(width*height*components);

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
        image_view.dataSize = static_cast<std::size_t>(width*height*components);

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
    ShaderPipeline sprite_shader;
    if ((sprite_shader.vs = load_shader(LLGL::ShaderType::Vertex, "sprite", &SpriteVertexFormat(), shader_defs)) == nullptr)
        return false;
    if ((sprite_shader.ps = load_shader(LLGL::ShaderType::Fragment, "sprite", nullptr, shader_defs)) == nullptr)
        return false;

    ShaderPipeline tilemap_shader;
    if ((tilemap_shader.vs = load_shader(LLGL::ShaderType::Vertex, "tilemap", &TilemapVertexFormat(), shader_defs)) == nullptr)
        return false;
    if ((tilemap_shader.ps = load_shader(LLGL::ShaderType::Fragment, "tilemap", nullptr, shader_defs)) == nullptr)
        return false;
    if ((tilemap_shader.gs = load_shader(LLGL::ShaderType::Geometry, "tilemap", &TilemapVertexFormat(), shader_defs)) == nullptr)
        return false;

    state.shaders[ShaderAssetKey::SpriteShader] = sprite_shader;
    state.shaders[ShaderAssetKey::TilemapShader] = tilemap_shader;

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
    ASSERT(state.textures.find(key) != state.textures.end(), "Key not found");
    return state.textures[key];
}

const TextureAtlas& Assets::GetTextureAtlas(AssetKey key) {
    ASSERT(state.textures_atlases.find(key) != state.textures_atlases.end(), "Key not found");
    return state.textures_atlases[key];
}

const Texture& Assets::GetItemTexture(size_t index) {
    ASSERT(state.items.find(index) != state.items.end(), "Key not found");
    return state.items[index];
}

const ShaderPipeline& Assets::GetShader(ShaderAssetKey key) {
    ASSERT(state.shaders.find(key) != state.shaders.end(), "Key not found");
    return state.shaders[key];
}

LLGL::Sampler& Assets::GetSampler(size_t index) {
    ASSERT(index < state.samplers.size(), "Index is out of bounds");
    return *state.samplers[index];
}

Texture create_texture(uint32_t width, uint32_t height, uint32_t layers, uint32_t components, int sampler, uint8_t* data) {
    LLGL::TextureDescriptor texture_desc;
    texture_desc.extent = LLGL::Extent3D(width, height, 1);
    texture_desc.bindFlags = LLGL::BindFlags::Sampled;
    texture_desc.cpuAccessFlags = 0;
    texture_desc.miscFlags = 0;

    LLGL::ImageView image_view;
    image_view.format = (components == 4 ? LLGL::ImageFormat::RGBA : LLGL::ImageFormat::RGB);
    image_view.dataType = LLGL::DataType::UInt8;
    image_view.data = data;
    image_view.dataSize = static_cast<std::size_t>(width*height*components);

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
    LLGL::ShaderType shader_type,
    const std::string& name,
    const LLGL::VertexFormat* vertex_format = nullptr,
    const std::vector<ShaderDef>& shader_defs = {}
) {
    std::string extension;

#if defined(BACKEND_VULKAN) || defined(BACKEND_OPENGL)
    switch (shader_type) {
        case LLGL::ShaderType::Vertex: extension = ".vert";
        break;
        case LLGL::ShaderType::Fragment: extension = ".frag";
        break;
        case LLGL::ShaderType::Geometry: extension = ".geom";
        break;
        default: break;
    };
#elif defined(BACKEND_D3D11)
    extension = ".hlsl";
#elif defined(BACKEND_METAL)
    extension = ".metal";
#endif

#if defined(BACKEND_OPENGL)
    std::string path = "assets/shaders/opengl/" + name + extension;
#elif defined(BACKEND_D3D11)
    std::string path = "assets/shaders/d3d11/" + name + extension;
#elif defined(BACKEND_METAL)
    std::string path = "assets/shaders/metal/" + name + extension;
#elif defined(BACKEND_VULKAN)
    std::string path = "assets/shaders/vulkan/" + name + extension;
#endif

    if (!FileExists(path.c_str())) {
        LOG_ERROR("Failed to find shader '%s'", path.c_str());
        return nullptr;
    }

    std::ifstream shader_file;
    shader_file.open(path);

    std::stringstream shader_source_str;
    shader_source_str << shader_file.rdbuf();

    std::string shader_source = shader_source_str.str();

    for (ShaderDef shader_def : shader_defs) {
        size_t pos;
        while ((pos = shader_source.find(shader_def.name)) != std::string::npos) {
            shader_source.replace(pos, shader_def.name.length(), shader_def.value);
        }
    }

    shader_file.close();

    LLGL::ShaderDescriptor shader_desc;
    shader_desc.type = shader_type;
    shader_desc.sourceType = LLGL::ShaderSourceType::CodeString;

    if (shader_type == LLGL::ShaderType::Vertex) {
        shader_desc.vertex.inputAttribs = vertex_format->attributes;
    }

#if defined(BACKEND_VULKAN)
    std::string output_path = path + ".spv";
    if (!FileExists(output_path.c_str())) {
        if (!CompileVulkanShader(shader_type, shader_source.c_str(), shader_source.length(), output_path.c_str())) {
            return nullptr;
        }
    }

    shader_desc.source = output_path.c_str();
    shader_desc.sourceType = LLGL::ShaderSourceType::BinaryFile;
#else
    shader_desc.type = shader_type;
    shader_desc.source = shader_source.c_str();
    shader_desc.sourceSize = shader_source.length();

#if defined(BACKEND_D3D11)
    switch (shader_type) {
        case LLGL::ShaderType::Vertex: {
            shader_desc.entryPoint = "VS";
            shader_desc.profile = "vs_4_0";
        }
        break;
        case LLGL::ShaderType::Fragment: {
            shader_desc.entryPoint = "PS";
            shader_desc.profile = "ps_4_0";
        }
        break;
        case LLGL::ShaderType::Geometry: {
            shader_desc.entryPoint = "GS";
            shader_desc.profile = "gs_4_0";
        }
        break;
        default: UNREACHABLE()
    }
#elif defined(BACKEND_METAL)
    shader_desc.flags |= LLGL::ShaderCompileFlags::DefaultLibrary;
    switch (shader_type) {
        case LLGL::ShaderType::Vertex: {
            shader_desc.entryPoint = "VS";
            shader_desc.profile = "1.1";
        }
        break;
        case LLGL::ShaderType::Fragment: {
            shader_desc.entryPoint = "PS";
            shader_desc.profile = "1.1";
        }
        break;
        case LLGL::ShaderType::Geometry: {
            shader_desc.entryPoint = "GS";
            shader_desc.profile = "1.1";
        }
        break;
        default: UNREACHABLE()
    }
#endif

#endif

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
            } else {
                LOG_INFO("%s", report->GetText());
            }
        }
    }

    return shader;
}