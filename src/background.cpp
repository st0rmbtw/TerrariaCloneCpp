#include "background.hpp"

#include <vector>

#include "assets.hpp"
#include "renderer/renderer.hpp"
#include "types/texture.hpp"

struct Layer {
    Sprite sprite;
    glm::vec2 speed = glm::vec2(0.0f);
    glm::vec2 initial_pos;
    float initial_scale;
    bool fill_screen_height = false;
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
        .sprite = Sprite()
            .set_texture(Assets::GetTexture(TextureKey::Background93))
            .set_nonscalable(true)
            .set_scale(2.0f),
        .speed = glm::vec2(0.2f, 0.4f),
        .initial_pos = glm::vec2(0.0f, (world.layers().underground - world.layers().dirt_height) * Constants::TILE_SIZE),
        .initial_scale = 2.0f
    });

    state.layers.push_back(Layer {
        .sprite = Sprite()
            .set_texture(Assets::GetTexture(TextureKey::Background114))
            .set_nonscalable(true)
            .set_scale(2.0f),
        .speed = glm::vec2(0.4f, 0.5f),
        .initial_pos = glm::vec2(0.0f, (world.layers().underground - world.layers().dirt_height) * Constants::TILE_SIZE),
        .initial_scale = 2.0f
    });

    state.layers.push_back(Layer {
        .sprite = Sprite()
            .set_texture(Assets::GetTexture(TextureKey::Background55))
            .set_anchor(Anchor::BottomCenter)
            .set_nonscalable(true)
            .set_scale(2.5f),
        .speed = glm::vec2(0.8f, 0.6f),
        .initial_pos = glm::vec2(0.0f, (world.layers().underground + world.layers().dirt_height * 0.5f) * Constants::TILE_SIZE),
        .initial_scale = 2.5f
    });
}

void Background::Update(const Camera &camera) {
    for (Layer& layer : state.layers) {
        layer.sprite.set_position(camera.position() + (layer.initial_pos - camera.position()) * layer.speed);
    }
}

void Background::Render() {
    for (const Layer& layer : state.layers) {
        Renderer::DrawSprite(layer.sprite);
    }
}