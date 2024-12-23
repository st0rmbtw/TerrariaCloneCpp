#include "game.hpp"

#include <GLFW/glfw3.h>

#include "constants.hpp"
#include "engine.hpp"
#include "glm/gtc/random.hpp"
#include "renderer/camera.h"
#include "renderer/renderer.hpp"
#include "time/time.hpp"
#include "assets.hpp"
#include "input.hpp"
#include "types/window_settings.hpp"
#include "ui.hpp"
#include "world/autotile.hpp"
#include "player/player.hpp"
#include "particles.hpp"
#include "background.hpp"
#include "defines.hpp"

static struct GameState {
    Camera camera;
    World world;
    Player player;
    bool free_camera = false;
} g;

glm::vec2 camera_follow_player() {
    static constexpr float PIXEL_OFFSET = 2.0f;

    glm::vec2 position = g.player.position();

    const math::Rect area = g.world.playable_area() * Constants::TILE_SIZE;
    const math::Rect& camera_area = g.camera.get_projection_area();
    
    const float left = camera_area.min.x;
    const float right = camera_area.max.x;

    const float top = camera_area.min.y;
    const float bottom = camera_area.max.y;

    if (position.x + left < area.min.x + PIXEL_OFFSET) position.x = area.min.x - left + PIXEL_OFFSET;
    if (position.x + right > area.max.x - PIXEL_OFFSET) position.x = area.max.x - right - PIXEL_OFFSET;

    if (position.y + top < area.min.y) position.y = area.min.y - top;
    if (position.y + bottom > area.max.y) position.y = area.max.y - bottom;

    return position;
}

#if DEBUG
glm::vec2 camera_free() {
    const float dt = Time::delta_seconds();
    glm::vec2 position = g.camera.position();

    float speed = 2000.0f;

    if (Input::Pressed(Key::LeftShift)) speed *= 2.0f;
    if (Input::Pressed(Key::LeftAlt)) speed /= 5.0f;

    if (Input::Pressed(Key::A)) position.x -= speed * dt;
    if (Input::Pressed(Key::D)) position.x += speed * dt;
    if (Input::Pressed(Key::W)) position.y -= speed * dt;
    if (Input::Pressed(Key::S)) position.y += speed * dt;

    const math::Rect& camera_area = g.camera.get_projection_area();

    if (position.y + camera_area.min.y < 0.0f) position.y = 0.0f - camera_area.min.y;

    return position;
}
#endif

void pre_update() {
    ParticleManager::DeleteExpired();

    UI::PreUpdate(g.player.inventory());

    g.player.pre_update();

    if (Input::JustPressed(Key::F)) g.free_camera = !g.free_camera;
}

void fixed_update() {
    ParticleManager::Update();

#if DEBUG
    const bool handle_input = !g.free_camera;
#else
    constexpr bool handle_input = true;
#endif

    g.player.fixed_update(g.world, handle_input);
}

void update() {
    float scale_speed = 2.f;

    if (Input::Pressed(Key::LeftShift)) {
        scale_speed *= 4.f;
    }

    if (Input::Pressed(Key::LeftAlt)) {
        scale_speed /= 4.f;
    }

    if (Input::Pressed(Key::Minus)) {
        g.camera.set_zoom(g.camera.zoom() + scale_speed * Time::delta_seconds());
    }

    if (Input::Pressed(Key::Equals)) {
        g.camera.set_zoom(g.camera.zoom() - scale_speed * Time::delta_seconds());
    }

#if DEBUG
    if (g.free_camera && Input::Pressed(MouseButton::Right)) {
        g.player.set_position(g.world, g.camera.screen_to_world(Input::MouseScreenPosition()));
    }
    const glm::vec2 position = g.free_camera ? camera_free() : camera_follow_player();
#else
    const glm::vec2 position = camera_follow_player();
#endif

    g.camera.set_position(position);

    g.camera.update();
    g.world.update(g.camera);

    Background::Update(g.camera);
    
    UI::Update(g.player.inventory());
    
    g.player.update(g.camera, g.world);

    if (Input::Pressed(Key::K)) {
        for (int i = 0; i < 500; ++i) {
            const glm::vec2 position = g.camera.screen_to_world(Input::MouseScreenPosition());
            const glm::vec2 velocity = glm::diskRand(1.0f) * 1.5f;

            ParticleManager::SpawnParticle(
                ParticleBuilder::create(Particle::Type::Grass, position, velocity, 5.0f)
                    .with_rotation_speed(glm::pi<float>() / 12.0f)
            );
        }
    }
}

void post_update() {
    UI::PostUpdate();
}

void render() {
    Renderer::Begin(g.camera, g.world.data());

    Background::Draw();

    g.world.draw();

    g.player.draw();

    ParticleManager::Draw();

    UI::Draw(g.camera, g.player.inventory());

    Renderer::Render(g.camera, g.world.chunk_manager());
}

void post_render() {
    if (g.world.chunk_manager().any_chunks_to_destroy()) {
        Renderer::CommandQueue()->WaitIdle();
        g.world.chunk_manager().destroy_hidden_chunks();
    }

#if DEBUG
    if (Input::Pressed(Key::C)) {
        Renderer::PrintDebugInfo();
    }
#endif
}

void window_resized(uint32_t width, uint32_t height, uint32_t scaled_width, uint32_t scaled_height) {
    Renderer::ResizeTextures(LLGL::Extent2D(scaled_width, scaled_height));

    g.camera.set_viewport(glm::uvec2(width, height));
    g.camera.update();
}

bool load_assets() {
    if (!Assets::Load()) return false;
    if (!Assets::LoadFonts()) return false;
    if (!Assets::InitSamplers()) return false;

    const std::vector<ShaderDef> shader_defs = {
        ShaderDef("TILE_SIZE", std::to_string(Constants::TILE_SIZE)),
        ShaderDef("WALL_SIZE", std::to_string(Constants::WALL_SIZE)),
    };

    if (!Assets::LoadShaders(shader_defs)) return false;

    return true;
}

void destroy() {
    g.world.data().lightmap_tasks_wait();
    g.world.chunk_manager().destroy();
    ParticleManager::Terminate();
}

bool Game::Init(RenderBackend backend, GameConfig config) {
    Engine::SetLoadAssetsCallback(load_assets);
    Engine::SetPreUpdateCallback(pre_update);
    Engine::SetUpdateCallback(update);
    Engine::SetPostUpdateCallback(post_update);
    Engine::SetFixedUpdateCallback(fixed_update);
    Engine::SetRenderCallback(render);
    Engine::SetPostRenderCallback(post_render);
    Engine::SetDestroyCallback(destroy);
    Engine::SetWindowResizeCallback(window_resized);

    const glm::uvec2 window_size = glm::uvec2(1280, 720);

    WindowSettings settings;
    settings.width = window_size.x;
    settings.height = window_size.y;
    settings.fullscreen = config.fullscreen;
    settings.hidden = true;

    if (!Engine::Init(backend, config.vsync, settings)) return false;

    Time::set_fixed_timestep_seconds(Constants::FIXED_UPDATE_INTERVAL);

    Engine::HideCursor();

    init_tile_rules();

    g.world.generate(200, 500, 0);

    Renderer::InitWorldRenderer(g.world.data());

    ParticleManager::Init();
    UI::Init();
    Background::SetupWorldBackground(g.world);

    g.camera.set_viewport({window_size.x, window_size.y});
    g.camera.set_zoom(1.0f);

    g.player.init();
    Inventory& inventory = g.player.inventory();
    inventory.set_item(0, ITEM_COPPER_AXE);
    inventory.set_item(1, ITEM_COPPER_PICKAXE);
    inventory.set_item(2, ITEM_COPPER_HAMMER);
    inventory.set_item(3, ITEM_DIRT_BLOCK.with_max_stack());
    inventory.set_item(4, ITEM_STONE_BLOCK.with_max_stack());
    inventory.set_item(5, ITEM_WOOD_BLOCK.with_max_stack());
    g.player.set_position(g.world, glm::vec2(g.world.spawn_point()) * Constants::TILE_SIZE);

    Engine::ShowWindow();

    return true;
}

void Game::Run() {
    Engine::Run();
}

void Game::Destroy() {
    Engine::Destroy();
}