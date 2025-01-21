#include "game.hpp"

#include <GLFW/glfw3.h>

#include <glm/gtc/random.hpp>

#include "constants.hpp"
#include "engine.hpp"
#include "renderer/camera.h"
#include "renderer/renderer.hpp"
#include "time/time.hpp"
#include "assets.hpp"
#include "input.hpp"
#include "types/window_settings.hpp"
#include "ui/ui.hpp"
#include "world/autotile.hpp"
#include "player/player.hpp"
#include "particles.hpp"
#include "background.hpp"
#include "defines.hpp"

#include <string>
#include <tracy/Tracy.hpp>

static struct GameState {
    Player player;
    World world;
    Camera camera;
    bool free_camera = false;
} g;

static glm::vec2 camera_follow_player() {
    static constexpr float OFFSET = 2.0f;

    glm::vec2 position = g.player.draw_position();

    const math::Rect area = g.world.playable_area() * Constants::TILE_SIZE;
    const math::Rect& camera_area = g.camera.get_projection_area();
    
    const float left = camera_area.min.x;
    const float right = camera_area.max.x;

    const float top = camera_area.min.y;
    const float bottom = camera_area.max.y;

    if (position.x + left < area.min.x + OFFSET) position.x = area.min.x - left + OFFSET;
    if (position.x + right > area.max.x - OFFSET) position.x = area.max.x - right - OFFSET;

    if (position.y + top < area.min.y) position.y = area.min.y - top;
    if (position.y + bottom > area.max.y) position.y = area.max.y - bottom;

    return position;
}

#if DEBUG
static glm::vec2 camera_free() {
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
    ZoneScopedN("Game::pre_update");

    ParticleManager::DeleteExpired();

    UI::PreUpdate(g.player.inventory());

    g.player.pre_update();
    g.world.clear_lights();

    if (Input::JustPressed(Key::F)) g.free_camera = !g.free_camera;
}

void fixed_update() {
    ZoneScopedN("Game::fixed_update");

    ParticleManager::Update(g.world);

#if DEBUG
    const bool handle_input = !g.free_camera;
#else
    constexpr bool handle_input = true;
#endif

    g.player.fixed_update(g.world, handle_input);

    UI::FixedUpdate();
}

void update() {
    ZoneScopedN("Game::update");

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

    Background::Update(g.camera, g.world);
    
    UI::Update(g.player.inventory());
    
    g.player.update(g.camera, g.world);

    if (Input::Pressed(Key::K)) {
        for (int i = 0; i < 500; ++i) {
            const glm::vec2 position = g.camera.screen_to_world(Input::MouseScreenPosition());
            const glm::vec2 velocity = glm::diskRand(1.0f) * 1.5f;

            ParticleManager::SpawnParticle(
                ParticleBuilder::create(Particle::Type::Grass, position, velocity, 5.0f)
                    .with_rotation_speed(glm::pi<float>() / 12.0f)
                    .with_light(glm::vec3(0.1f, 0.9f, 0.1f))
            );
        }
    }
}

void post_update() {
    ZoneScopedN("Game::post_update");

    UI::PostUpdate();
}

void render() {
    ZoneScopedN("Game::render");

    Renderer::Begin(g.camera, g.world.data());

    Background::Draw();

    g.world.draw();

    g.player.draw();

    ParticleManager::Draw();

    UI::Draw(g.camera, g.player);

    Renderer::Render(g.camera, g.world);
}

void post_render() {
    ZoneScopedN("Game::post_render");

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

    g.world.chunk_manager().manage_chunks(g.world.data(), g.camera);

    Background::Update(g.camera, g.world);
}

bool load_assets() {
    if (!Assets::Load()) return false;
    if (!Assets::LoadFonts()) return false;
    if (!Assets::InitSamplers()) return false;

    const std::vector<ShaderDef> shader_defs = {
        ShaderDef("TILE_SIZE", std::to_string(Constants::TILE_SIZE)),
        ShaderDef("WALL_SIZE", std::to_string(Constants::WALL_SIZE)),
        ShaderDef("DEF_SUBDIVISION", std::to_string(Constants::SUBDIVISION)),
        ShaderDef("DEF_SOLID_DECAY", std::to_string(Constants::LightDecay(true))),
        ShaderDef("DEF_AIR_DECAY", std::to_string(Constants::LightDecay(false))),
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
    ZoneScopedN("Game::Init");

    Engine::SetLoadAssetsCallback(load_assets);
    Engine::SetPreUpdateCallback(pre_update);
    Engine::SetUpdateCallback(update);
    Engine::SetPostUpdateCallback(post_update);
    Engine::SetFixedUpdateCallback(fixed_update);
    Engine::SetRenderCallback(render);
    Engine::SetPostRenderCallback(post_render);
    Engine::SetDestroyCallback(destroy);
    Engine::SetWindowResizeCallback(window_resized);

    WindowSettings settings;
    settings.width = 1280;
    settings.height = 720;
    settings.fullscreen = config.fullscreen;
    settings.hidden = true;

    glm::uvec2 resolution;
    if (!Engine::Init(backend, config.vsync, settings, &resolution)) return false;

    Time::set_fixed_timestep_seconds(Constants::FIXED_UPDATE_INTERVAL);

    Engine::HideCursor();

    init_tile_rules();

    g.world.generate(200, 500, 0);

    g.camera.set_viewport(resolution);
    g.camera.set_zoom(1.0f);

    Renderer::InitWorldRenderer(g.world.data());

    ParticleManager::Init();
    UI::Init();

    g.player.init();
    g.player.set_position(g.world, glm::vec2(g.world.spawn_point()) * Constants::TILE_SIZE);
    g.camera.set_position(g.player.draw_position());

    Background::SetupWorldBackground(g.world);

    Inventory& inventory = g.player.inventory();
    inventory.set_item(0, ITEM_COPPER_AXE);
    inventory.set_item(1, ITEM_COPPER_PICKAXE);
    inventory.set_item(2, ITEM_COPPER_HAMMER);
    inventory.set_item(3, ITEM_DIRT_BLOCK.with_max_stack());
    inventory.set_item(4, ITEM_STONE_BLOCK.with_max_stack());
    inventory.set_item(5, ITEM_WOOD_BLOCK.with_max_stack());
    inventory.set_item(6, ITEM_TORCH.with_max_stack());

    Engine::ShowWindow();

    return true;
}

void Game::Run() {
    Engine::Run();
}

void Game::Destroy() {
    Engine::Destroy();
}