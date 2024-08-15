#include "background.hpp"

#include <vector>

#include "assets.hpp"
#include "constants.hpp"
#include "types/anchor.hpp"
#include "types/background_layer.hpp"
#include "renderer/renderer.hpp"

using Constants::TILE_SIZE;

void setup_cavern_background(const World& world);
void setup_sky_background();

static struct BackgroundState {
    std::vector<BackgroundLayer> layers;
} state;

void Background::SetupMenuBackground() {
    const float pos = 150.0f;

    setup_sky_background();

    state.layers.push_back(
        BackgroundLayer(TextureAsset::Background7, 1.5f)
            .set_y(pos)
            .set_speed(1.0f, 0.9f)
    );
    state.layers.push_back(
        BackgroundLayer(TextureAsset::Background90, 1.5f)
            .set_speed(1.0f, 0.8f)
            .set_y(pos - 200.0f)
    );
    state.layers.push_back(
        BackgroundLayer(TextureAsset::Background91, 1.5f)
            .set_speed(1.0f, 0.7f)
            .set_y(pos - 300.0f)
    );
    state.layers.push_back(
        BackgroundLayer(TextureAsset::Background92, 1.5f)
            .set_speed(1.0f, 0.6f)
            .set_y(0.0f)
    );
    state.layers.push_back(
        BackgroundLayer(TextureAsset::Background112, 1.2f)
            .set_speed(1.0f, 0.7f)
            .set_y(0.0f)
    );
}

void Background::SetupWorldBackground(const World& world) {
    state.layers.clear();

    setup_sky_background();

    state.layers.push_back(
        BackgroundLayer(TextureAsset::Background93, 2.0f)
            .set_speed(0.1f, 0.4f)
            .set_y(world.layers().underground * TILE_SIZE)
            .set_anchor(Anchor::BottomCenter)
            .set_fill_screen_width()
    );

    state.layers.push_back(
        BackgroundLayer(TextureAsset::Background114, 2.0f)
            .set_speed(0.15f, 0.5f)
            .set_y((world.layers().underground + world.layers().dirt_height * 0.5f) * TILE_SIZE)
            .set_anchor(Anchor::BottomCenter)
            .set_fill_screen_width()
    );

    state.layers.push_back(
        BackgroundLayer(TextureAsset::Background55, 2.0f)
            .set_speed(0.2f, 0.6f)
            .set_y((world.layers().underground + world.layers().dirt_height * 0.5f) * TILE_SIZE)
            .set_anchor(Anchor::BottomCenter)
            .set_fill_screen_width()
    );

    setup_cavern_background(world);
}

void Background::Update(const Camera &camera) {
    for (BackgroundLayer& layer : state.layers) {
        glm::vec2 new_position = layer.position();

        new_position.x = layer.follow_camera() ? camera.position().x : layer.x();
        new_position.y = camera.position().y + (layer.y() - camera.position().y) * layer.speed().y;

        layer.set_position(new_position);

        if (layer.fill_screen_height()) {
            layer.set_height(camera.viewport().y);
        }
        if (layer.fill_screen_width()) {
            layer.set_width(camera.viewport().x);
        }
    }
}

void Background::Render() {
    for (const BackgroundLayer& layer : state.layers) {
        Renderer::DrawBackground(layer);
    }
}

void setup_sky_background() {
    state.layers.push_back(
        BackgroundLayer(TextureAsset::Background0, 1.0f)
            .set_anchor(Anchor::Center)
            .set_speed(0.0f, 0.0f)
            .set_y(0.0f)
            .set_fill_screen_height()
            .set_fill_screen_width()
            .set_nonscale(true)
            .set_follow_camera(true)
    );
}

void setup_cavern_background(const World& world) {
    const float underground_level = static_cast<float>(world.layers().underground) * TILE_SIZE + TILE_SIZE;
    const float world_height = static_cast<float>(world.area().height() - (world.area().height() - world.playable_area().height()) / 2) * TILE_SIZE;
    const float world_width = static_cast<float>(world.playable_area().width()) * TILE_SIZE;

    const float speed_x = 0.5f;

    state.layers.push_back(
        BackgroundLayer(TextureAsset::Background77, 1.0f)
            .set_speed(speed_x, 1.0f)
            .set_anchor(Anchor::BottomLeft)
            .set_x(world.playable_area().left() * TILE_SIZE)
            .set_y(underground_level)
            .set_width(world_width)
            .set_nonscale(false)
            .set_follow_camera(false)
            .set_world_background()
    );

    state.layers.push_back(
        BackgroundLayer(TextureAsset::Background78, 1.0f)
            .set_speed(speed_x, 1.0f) 
            .set_anchor(Anchor::TopLeft)
            .set_x(world.playable_area().left() * TILE_SIZE)
            .set_y(underground_level)
            .set_width(world_width)
            .set_height(world_height - underground_level)
            .set_nonscale(false)
            .set_follow_camera(false)
            .set_world_background()
    );
}