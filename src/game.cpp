#include "game.hpp"

#include <string>

#include <GLFW/glfw3.h>
#include <LLGL/Types.h>
#include <glm/gtc/random.hpp>

#include <SGE/defines.hpp>
#include <SGE/engine.hpp>
#include <SGE/input.hpp>
#include <SGE/renderer/camera.hpp>
#include <SGE/time/time.hpp>
#include <SGE/types/cursor_mode.hpp>
#include <SGE/profile.hpp>

#include "renderer/renderer.hpp"
#include "ui/ui.hpp"
#include "world/autotile.hpp"

#include "player/player.hpp"
#include "particles.hpp"
#include "background.hpp"
#include "assets.hpp"
#include "constants.hpp"

static struct GameState {
    Player player;
    World world;
    sge::Camera camera = sge::Camera(sge::CameraOrigin::Center, sge::CoordinateSystem {
        .up = sge::CoordinateDirectionY::Negative,
        .forward = sge::CoordinateDirectionZ::Negative,
    });
    std::vector<Light> lights;
    bool free_camera = false;
} g;

static glm::vec2 camera_follow_player() {
    static constexpr float OFFSET = Constants::WORLD_BOUNDARY_OFFSET;

    glm::vec2 position = g.player.draw_position();

    const sge::Rect area = g.world.playable_area() * Constants::TILE_SIZE;
    const sge::Rect& camera_area = g.camera.get_projection_area();

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

#if DEBUG_TOOLS
static glm::vec2 camera_free() {
    const float dt = sge::Time::DeltaSeconds();
    glm::vec2 position = g.camera.position();

    float speed = 2000.0f;

    if (sge::Input::Pressed(sge::Key::LeftShift)) speed *= 2.0f;
    if (sge::Input::Pressed(sge::Key::LeftAlt)) speed /= 5.0f;

    if (sge::Input::Pressed(sge::Key::A)) position.x -= speed * dt;
    if (sge::Input::Pressed(sge::Key::D)) position.x += speed * dt;
    if (sge::Input::Pressed(sge::Key::W)) position.y -= speed * dt;
    if (sge::Input::Pressed(sge::Key::S)) position.y += speed * dt;

    const sge::Rect& camera_area = g.camera.get_projection_area();

    if (position.y + camera_area.min.y < 0.0f) position.y = 0.0f - camera_area.min.y;

    return position;
}
#endif

static void pre_update() {
    ZoneScoped;

#if DEBUG
    if (sge::Input::JustPressed(sge::Key::B)) {
        SGE_DEBUG_BREAK();
    }
#endif

    ParticleManager::DeleteExpired();

    UI::PreUpdate(g.player.inventory());

    g.player.pre_update();
    g.world.clear_lights();

#if DEBUG_TOOLS
    if (sge::Input::JustPressed(sge::Key::F)) g.free_camera = !g.free_camera;
#endif
}

static void fixed_update() {
    ZoneScoped;

#if DEBUG_TOOLS
    const bool handle_input = !g.free_camera;
#else
    constexpr bool handle_input = true;
#endif

    g.player.fixed_update(g.camera, g.world, handle_input);

    UI::FixedUpdate();

    GameRenderer::UpdateLight();

    ParticleManager::Update(g.world);
}

static void update() {
    ZoneScoped;

    float scale_speed = 2.f;

    if (sge::Input::Pressed(sge::Key::LeftShift)) scale_speed *= 4.f;
    if (sge::Input::Pressed(sge::Key::LeftAlt)) scale_speed /= 4.f;

    if (sge::Input::Pressed(sge::Key::Minus)) {
        const float zoom = g.camera.zoom() + scale_speed * sge::Time::DeltaSeconds();
        g.camera.set_zoom(glm::clamp(zoom, Constants::CAMERA_MAX_ZOOM, Constants::CAMERA_MIN_ZOOM));
    }

    if (sge::Input::Pressed(sge::Key::Equals)) {
        const float zoom = g.camera.zoom() - scale_speed * sge::Time::DeltaSeconds();
        g.camera.set_zoom(glm::clamp(zoom, Constants::CAMERA_MAX_ZOOM, Constants::CAMERA_MIN_ZOOM));
    }

#if DEBUG_TOOLS
    if (g.free_camera && sge::Input::Pressed(sge::MouseButton::Right)) {
        g.player.set_position(g.world, g.camera.screen_to_world(sge::Input::MouseScreenPosition()));
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

    g.player.update(g.world);

    g.world.add_light(Light {
        .color = glm::vec3(1.0f, 0.0f, 0.0f),
        .pos = get_lightmap_pos(g.camera.screen_to_world(sge::Input::MouseScreenPosition())),
        .size = glm::uvec2(2)
    });

    if (sge::Input::JustPressed(sge::Key::Q)) {
        g.lights.push_back(Light {
            .color = glm::linearRand(glm::vec3(0.0f), glm::vec3(1.0f)),
            .pos = get_lightmap_pos(g.camera.screen_to_world(sge::Input::MouseScreenPosition())),
            .size = glm::uvec2(2)
        });
    }

    for (const Light& light : g.lights) {
        g.world.add_light(light);
    }

#if DEBUG_TOOLS
    if (sge::Input::Pressed(sge::Key::K)) {
        const glm::vec2 position = g.camera.screen_to_world(sge::Input::MouseScreenPosition());

        for (int i = 0; i < 500; ++i) {
            const glm::vec2 velocity = glm::diskRand(1.5f);

            ParticleManager::SpawnParticle(
                ParticleBuilder::create(Particle::Type::Grass, position, velocity, 5.0f)
                    .with_rotation_speed(glm::pi<float>() / 12.0f)
                    .with_light(glm::vec3(0.1f, 0.9f, 0.1f))
            );
        }
    }
#endif
}

static void post_update() {
    ZoneScoped;

    UI::PostUpdate();
}

static void render() {
    ZoneScoped;

    GameRenderer::Begin(g.camera, g.world);

    Background::Draw();

    g.world.draw(g.camera);

    g.player.draw();

    ParticleManager::Draw();

    UI::Draw(g.camera, g.player);

    GameRenderer::Render(g.camera, g.world);
}

static void post_render() {
    ZoneScoped;

    if (g.world.chunk_manager().any_chunks_to_destroy()) {
        sge::Engine::Renderer().CommandQueue()->WaitIdle();
        g.world.chunk_manager().destroy_hidden_chunks();
    }

#if DEBUG
    if (sge::Input::Pressed(sge::Key::C)) {
        sge::Engine::Renderer().PrintDebugInfo();
    }
#endif
}

static void window_resized(uint32_t width, uint32_t height, uint32_t scaled_width, uint32_t scaled_height) {
    GameRenderer::ResizeTextures(LLGL::Extent2D(scaled_width, scaled_height));

    g.camera.set_viewport(glm::uvec2(width, height));
    g.camera.update();

    g.world.chunk_manager().manage_chunks(g.world.data(), g.camera);

    Background::Update(g.camera, g.world);

    render();
}

bool load_assets() {
    if (!Assets::Load()) return false;
    if (!Assets::LoadFonts()) return false;

    const std::vector<sge::ShaderDef> shader_defs = {
        sge::ShaderDef("TILE_SIZE", std::to_string(Constants::TILE_SIZE)),
        sge::ShaderDef("WALL_SIZE", std::to_string(Constants::WALL_SIZE)),
        sge::ShaderDef("DEF_SUBDIVISION", std::to_string(Constants::SUBDIVISION)),
        sge::ShaderDef("DEF_SOLID_DECAY", std::to_string(Constants::LightDecay(true))),
        sge::ShaderDef("DEF_AIR_DECAY", std::to_string(Constants::LightDecay(false))),
    };

    if (!Assets::LoadShaders(shader_defs)) return false;

    return true;
}

static void destroy() {
    g.world.data().lightmap_tasks_wait();
    g.world.chunk_manager().destroy();
}

bool Game::Init(sge::RenderBackend backend, AppConfig config, int16_t world_width, int16_t world_height) {
    ZoneScoped;

    sge::Engine::SetLoadAssetsCallback(load_assets);
    sge::Engine::SetPreUpdateCallback(pre_update);
    sge::Engine::SetUpdateCallback(update);
    sge::Engine::SetPostUpdateCallback(post_update);
    sge::Engine::SetFixedUpdateCallback(fixed_update);
    sge::Engine::SetRenderCallback(render);
    sge::Engine::SetPostRenderCallback(post_render);
    sge::Engine::SetDestroyCallback(destroy);
    sge::Engine::SetWindowResizeCallback(window_resized);

    sge::EngineConfig engine_config;
    engine_config.cache_pipelines = true;
    engine_config.window_settings.title = "TerrariaClone";
    engine_config.window_settings.width = 1280;
    engine_config.window_settings.height = 720;
    engine_config.window_settings.cursor_mode = CursorMode::Hidden;
    engine_config.window_settings.samples = config.samples;
    engine_config.window_settings.fullscreen = config.fullscreen;
    engine_config.window_settings.vsync = config.vsync;
    engine_config.window_settings.samples = config.samples;
    engine_config.window_settings.hidden = true;

    LLGL::Extent2D resolution;
    if (!sge::Engine::Init(backend, engine_config, resolution)) return false;

    if (!GameRenderer::Init(resolution)) return false;

    sge::Time::SetFixedTimestepSeconds(Constants::FIXED_UPDATE_INTERVAL);

    init_tile_rules();

    g.world.generate(world_width, world_height, 0);

    g.camera.set_viewport(glm::uvec2(resolution.width, resolution.height));
    g.camera.set_zoom(1.0f);

    GameRenderer::InitWorldRenderer(g.world.data());

    ParticleManager::Init();
    UI::Init();

    g.player.init();
    g.player.set_position(g.world, glm::vec2(g.world.spawn_point()) * Constants::TILE_SIZE);
    g.camera.set_position(g.player.draw_position());

    Background::SetupWorldBackground(g.world);

    Inventory& inventory = g.player.inventory();
    inventory.add_item_stack(ITEM_COPPER_AXE);
    inventory.add_item_stack(ITEM_COPPER_PICKAXE);
    inventory.add_item_stack(ITEM_COPPER_HAMMER);
    inventory.add_item_stack(ITEM_DIRT_BLOCK.with_stack(1000));
    inventory.add_item_stack(ITEM_DIRT_BLOCK.with_stack(2500));
    inventory.add_item_stack(ITEM_STONE_BLOCK.with_max_stack());
    inventory.add_item_stack(ITEM_WOOD_BLOCK.with_max_stack());
    inventory.add_item_stack(ITEM_TORCH.with_max_stack());
    inventory.add_item_stack(ITEM_WOOD_WALL.with_max_stack());
    inventory.add_item_stack(ITEM_DIRT_BLOCK.with_max_stack());

    sge::Engine::ShowWindow();

    return true;
}

void Game::Run() {
    sge::Engine::Run();
}

void Game::Destroy() {
    GameRenderer::Terminate();
    ParticleManager::Terminate();
    sge::Engine::Destroy();
}