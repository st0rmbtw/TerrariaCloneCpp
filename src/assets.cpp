#include <LLGL/Utils/VertexFormat.h>
#include <STB/stb_image.h>
#include "assets.hpp"
#include "renderer/assets.hpp"
#include "renderer/renderer.hpp"
#include "log.hpp"
#include "utils.hpp"

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
    // std::unordered_map<FontKey, Font> fonts;
    uint32_t texture_index;
} assets_state;

Texture create_texture(uint32_t width, uint32_t height, uint32_t components, int sampler, uint8_t* data);

bool Assets::Load() {
    uint8_t data[] = { 0xFF, 0xFF, 0xFF, 0xFF };
    assets_state.textures[AssetKey::TextureStub] = create_texture(1, 1, 4, TextureSampler::Nearest, data);

    for (auto& asset : TEXTURE_ASSETS) {
        int width, height, components;

        uint8_t* data = stbi_load(asset.second.path.c_str(), &width, &height, &components, 0);
        if (data == nullptr) {
            LOG_ERROR("Couldn't load asset: %s", asset.second.path.c_str());
            return false;   
        }

        assets_state.textures[asset.first] = create_texture(width, height, components, asset.second.sampler, data);

        stbi_image_free(data);
    }

    for (const auto& asset : TEXTURE_ATLAS_ASSETS) {
        assets_state.textures_atlases.emplace(asset.first, TextureAtlas::from_grid(Assets::GetTexture(asset.first), asset.second.tile_size, asset.second.columns, asset.second.rows, asset.second.padding, asset.second.offset));
    }

    return true;
}

Texture create_texture(uint32_t width, uint32_t height, uint32_t components, int sampler, uint8_t* data) {
    LLGL::TextureDescriptor texture_desc;
    texture_desc.extent = LLGL::Extent3D(width, height, 1);

    LLGL::ImageView image_view;
    image_view.format = (components == 4 ? LLGL::ImageFormat::RGBA : LLGL::ImageFormat::RGB);
    image_view.dataType = LLGL::DataType::UInt8;
    image_view.data = data;
    image_view.dataSize = static_cast<std::size_t>(width*height*components);

    Texture texture;
    texture.id = assets_state.texture_index++;
    texture.texture = Renderer::Context()->CreateTexture(texture_desc, &image_view);
    texture.sampler = sampler;

    return texture;
}

void Assets::DestroyTextures() {
    for (auto& entry : assets_state.textures) {
        Renderer::Context()->Release(*entry.second.texture);
    }
}

const Texture& Assets::GetTexture(AssetKey key) {
    return assets_state.textures[key];
}

const TextureAtlas& Assets::GetTextureAtlas(AssetKey key) {
    return assets_state.textures_atlases[key];
}