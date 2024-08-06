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
    glm::vec2 initial_pos;
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
        .initial_pos = glm::vec2(0.0f, (world.layers().underground + world.layers().dirt_height * 0.5f) * Constants::TILE_SIZE),
        .anchor = Anchor::BottomLeft
    });

    state.layers.push_back(Layer {
        .texture = Assets::GetTexture(TextureKey::Background114),
        .scale = 2.0f,
        .speed = glm::vec2(0.4f, 0.5f),
        .initial_pos = glm::vec2(0.0f, (world.layers().underground + world.layers().dirt_height * 0.5f) * Constants::TILE_SIZE),
        .anchor = Anchor::BottomLeft
    });

    state.layers.push_back(Layer {
        .texture = Assets::GetTexture(TextureKey::Background55),
        .scale = 2.5f,
        .speed = glm::vec2(0.8f, 0.6f),
        .initial_pos = glm::vec2(0.0f, (world.layers().underground + world.layers().dirt_height * 0.5f) * Constants::TILE_SIZE),
        .anchor = Anchor::BottomLeft
    });
}

void Background::Update(const Camera &camera) {
    for (Layer& layer : state.layers) {
        layer.position = camera.position() + (layer.initial_pos - camera.position()) * layer.speed;
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

        const glm::vec2 position = layer.position;
        const glm::vec2 texture_size = layer.texture.size();
        int count = static_cast<float>(camera.viewport().x) / (texture_size.x * layer.scale * 0.5f);
        count = glm::max(count, 1);

        for (int i = -count; i <= count; ++i) {
            sprite.set_position(position + glm::vec2(texture_size.x * i * 2.0f, 0.0f));

            Renderer::DrawSprite(sprite, depth);
        }

        depth = Renderer::GetGlobalDepthIndex();
    }
}