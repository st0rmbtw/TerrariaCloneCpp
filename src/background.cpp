#include "background.hpp"

#include <vector>

#include "assets.hpp"
#include "renderer/renderer.hpp"
#include "types/anchor.hpp"
#include "types/texture.hpp"

struct Layer {
    Texture texture;
    float scale = 1.0f;
    glm::vec2 speed = glm::vec2(0.0f);
    glm::vec2 position = glm::vec2(0.0f);
    float y;
    bool fill_screen_height = false;
    Anchor anchor = Anchor::Center;
};

static struct BackgroundState {
    std::vector<Layer> layers;
} state;

void Background::Init(const World& world) {
    state.layers.clear();

    // const float pos = 150.0f;

    // state.layers.push_back(Layer {
    //     .texture = Assets::GetTexture(TextureKey::Background0),
    //     .speed = glm::vec2(1.f, 0.f),
    //     .scale = 1.f,
    //     .y = 0.f,
    //     .fill_screen_height = true
    // });
    // state.layers.push_back(Layer {
    //     .texture = Assets::GetTexture(TextureKey::Background7),
    //     .speed = glm::vec2(1.f, 0.9f),
    //     .scale = 1.5f,
    //     .y = pos,
    // });
    // state.layers.push_back(Layer {
    //     .texture = Assets::GetTexture(TextureKey::Background90),
    //     .speed = glm::vec2(1.f, 0.8f),
    //     .scale = 1.5f,
    //     .y = pos - 200.0f,
    // });
    // state.layers.push_back(Layer {
    //     .texture = Assets::GetTexture(TextureKey::Background91),
    //     .speed = glm::vec2(1.f, 0.7f),
    //     .scale = 1.5f,
    //     .y = pos - 300.0f,
    // });
    // state.layers.push_back(Layer {
    //     .texture = Assets::GetTexture(TextureKey::Background92),
    //     .speed = glm::vec2(1.f, 0.6f),
    //     .scale = 1.5f,
    //     .y = 0.f,
    // });
    // state.layers.push_back(Layer {
    //     .texture = Assets::GetTexture(TextureKey::Background112),
    //     .speed = glm::vec2(1.f, 0.7f),
    //     .scale = 1.2f,
    //     .y = 0.f,
    // });

    state.layers.push_back(Layer {
        .texture = Assets::GetTexture(TextureKey::Background93),
        .scale = 2.0f,
        .speed = glm::vec2(0.2f, 0.4f),
        .y = world.layers().underground * Constants::TILE_SIZE,
        .anchor = Anchor::BottomCenter
    });

    state.layers.push_back(Layer {
        .texture = Assets::GetTexture(TextureKey::Background114),
        .scale = 2.0f,
        .speed = glm::vec2(0.4f, 0.5f),
        .y = (world.layers().underground + world.layers().dirt_height * 0.5f) * Constants::TILE_SIZE,
        .anchor = Anchor::BottomCenter
    });

    state.layers.push_back(Layer {
        .texture = Assets::GetTexture(TextureKey::Background55),
        .scale = 2.0f,
        .speed = glm::vec2(0.8f, 0.6f),
        .y = (world.layers().underground + world.layers().dirt_height * 0.5f) * Constants::TILE_SIZE,
        .anchor = Anchor::BottomCenter
    });
}

void Background::Update(const Camera &camera) {
    for (Layer& layer : state.layers) {
        const float texture_width = layer.texture.size.x;

        layer.position.y = camera.position().y + (layer.y - camera.position().y) * layer.speed.y;
        layer.position.x = camera.position().x + (0.0f - camera.position().x) * (1.0f - layer.speed.x);
        layer.position.x = camera.position().x - fmod(layer.position.x, texture_width * 2.0f);
    }
}

void Background::Render(const Camera& camera) {
    Sprite sprite;
    sprite.set_nonscalable(true);

    int depth = Renderer::GetGlobalDepthIndex();

    for (const Layer& layer : state.layers) {
        sprite.set_scale(layer.scale);
        sprite.set_anchor(layer.anchor);
        sprite.set_texture(layer.texture);

        const glm::vec2& position = layer.position;
        const float texture_width = layer.texture.size.x;

        int count = static_cast<float>(camera.viewport().x) / (texture_width * layer.scale * 0.5f) + 1;
        count = glm::max(count, 1);

        for (int i = -count; i <= count; ++i) {
            sprite.set_position(position + glm::vec2(texture_width * 2.0f * i, 0.0f));

            Renderer::DrawSprite(sprite, depth);
        }

        depth = Renderer::GetGlobalDepthIndex();
    }
}