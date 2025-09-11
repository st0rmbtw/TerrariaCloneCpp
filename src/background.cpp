#include "background.hpp"

#include <vector>

#include <SGE/profile.hpp>
#include <SGE/types/anchor.hpp>

#include "types/background_layer.hpp"
#include "renderer/renderer.hpp"

#include "assets.hpp"
#include "constants.hpp"

using Constants::TILE_SIZE;

static void setup_cavern_background(const World& world);
static void setup_sky_background(float y, bool fill_screen_height);

static struct BackgroundState {
    std::vector<BackgroundLayer> layers;
} state;

void Background::SetupMenuBackground() {
    state.layers.clear();

    const float pos = 150.0f;

    setup_sky_background(0.0f, true);

    state.layers.push_back(
        BackgroundLayer(BackgroundAsset::Background7, 1.5f)
            .set_y(pos)
            .set_speed(1.0f, 0.9f)
    );
    state.layers.push_back(
        BackgroundLayer(BackgroundAsset::Background90, 1.5f)
            .set_speed(1.0f, 0.8f)
            .set_y(pos - 200.0f)
    );
    state.layers.push_back(
        BackgroundLayer(BackgroundAsset::Background91, 1.5f)
            .set_speed(1.0f, 0.7f)
            .set_y(pos - 300.0f)
    );
    state.layers.push_back(
        BackgroundLayer(BackgroundAsset::Background92, 1.5f)
            .set_speed(1.0f, 0.6f)
            .set_y(0.0f)
    );
    state.layers.push_back(
        BackgroundLayer(BackgroundAsset::Background112, 1.2f)
            .set_speed(1.0f, 0.7f)
            .set_y(0.0f)
    );
}

void Background::SetupWorldBackground(const World& world) {
    state.layers.clear();

    const Layers world_layers = world.layers();

    setup_sky_background(world_layers.underground * TILE_SIZE - 1000.0f, false);

    state.layers.push_back(
        BackgroundLayer(BackgroundAsset::Background93, 2.0f)
            .set_speed(0.05f, 0.1f)
            .set_y(world_layers.underground * TILE_SIZE + 600.0f)
            .set_anchor(sge::Anchor::BottomCenter)
            .set_fill_screen_width(true)
            .set_surface_layer(true)
    );

    state.layers.push_back(
        BackgroundLayer(BackgroundAsset::Background114, 2.0f)
            .set_speed(0.1f, 0.15f)
            .set_y(world_layers.underground * TILE_SIZE + 600.0f)
            .set_anchor(sge::Anchor::BottomCenter)
            .set_fill_screen_width(true)
            .set_surface_layer(true)
    );

    state.layers.push_back(
        BackgroundLayer(BackgroundAsset::Background55, 2.3f)
            .set_speed(0.15f, 0.3f)
            .set_y(world_layers.underground * TILE_SIZE + 300.0f)
            .set_anchor(sge::Anchor::BottomCenter)
            .set_fill_screen_width(true)
            .set_surface_layer(true)
    );

    setup_cavern_background(world);
}

void Background::UpdateInGame(const sge::Camera &camera, const World& world) {
    ZoneScoped;

    const float offset = (camera.viewport().y - 600.0f) * 0.5f;

    for (BackgroundLayer& layer : state.layers) {
        glm::vec2 new_position = layer.position();

        const float layer_y = layer.is_surface_layer() ? layer.y() + offset : layer.y();
        const float off = (camera.position().y - world.layers().underground * TILE_SIZE) * (1.0f - layer.speed().y);

        new_position.x = layer.follow_camera() ? camera.position().x : layer.x();
        new_position.y = layer_y + off;

        layer.set_position(new_position);

        if (layer.fill_screen_height()) {
            layer.set_height(camera.viewport().y);
        }
        if (layer.fill_screen_width()) {
            layer.set_width(camera.viewport().x);
        }
    }
}

void Background::Draw() {
    ZoneScoped;

    for (const BackgroundLayer& layer : state.layers) {
        GameRenderer::DrawBackground(layer);
    }
}

static void setup_sky_background(float y, bool fill_screen_height) {
    state.layers.push_back(
        BackgroundLayer(BackgroundAsset::Background0, 2.0f)
            .set_anchor(sge::Anchor::TopCenter)
            .set_speed(0.0f, 0.08f)
            .set_y(y)
            .set_fill_screen_height(fill_screen_height)
            .set_fill_screen_width(true)
            .set_nonscale(true)
            .set_follow_camera(true)
            .set_surface_layer(true)
    );
}

static void setup_cavern_background(const World& world) {
    const float underground_level = static_cast<float>(world.layers().underground) * TILE_SIZE + TILE_SIZE;
    const float world_height = static_cast<float>(world.area().height() - (world.area().height() - world.playable_area().height()) / 2) * TILE_SIZE;
    const float world_width = static_cast<float>(world.playable_area().width()) * TILE_SIZE;

    const float speed_x = 0.5f;

    state.layers.push_back(
        BackgroundLayer(BackgroundAsset::Background77, 1.0f)
            .set_speed(speed_x, 1.0f)
            .set_anchor(sge::Anchor::BottomLeft)
            .set_x(world.playable_area().left() * TILE_SIZE)
            .set_y(underground_level)
            .set_width(world_width)
            .set_nonscale(false)
            .set_follow_camera(false)
            .set_world_background()
    );

    state.layers.push_back(
        BackgroundLayer(BackgroundAsset::Background78, 1.0f)
            .set_speed(speed_x, 1.0f) 
            .set_anchor(sge::Anchor::TopLeft)
            .set_x(world.playable_area().left() * TILE_SIZE)
            .set_y(underground_level)
            .set_width(world_width)
            .set_height(world_height - underground_level)
            .set_nonscale(false)
            .set_follow_camera(false)
            .set_world_background()
    );
}