#include "app.hpp"

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

#include "diagnostic/frametime.hpp"
#include "renderer/renderer.hpp"
#include "world/autotile.hpp"
#include "state/base.hpp"
#include "state/ingame.hpp"

#include "particles.hpp"
#include "assets.hpp"
#include "constants.hpp"

static BaseState* g_current_state = nullptr;
static glm::uvec2 g_window_size = glm::uvec2(100, 100);

static void PreUpdate() {
    FrameTime::Update(sge::Time::DeltaSeconds());

    g_current_state->PreUpdate();
}

static void FixedUpdate() {
    g_current_state->FixedUpdate();
}

static void Update() {
    g_current_state->Update();
}

static void PostUpdate() {
    g_current_state->PostUpdate();

    BaseState* new_state = g_current_state->GetNextState();
    if (new_state != nullptr && new_state != g_current_state) {
        delete g_current_state;
        g_current_state = new_state;
    }
}

static void Render() {
    g_current_state->Render();
}

static void PostRender() {
    g_current_state->PostRender();
}

static void OnWindowResized(uint32_t width, uint32_t height, uint32_t scaled_width, uint32_t scaled_height) {
    GameRenderer::ResizeTextures(LLGL::Extent2D(scaled_width, scaled_height));

    glm::uvec2 new_size = glm::uvec2(width, height);
    g_window_size = new_size;

    g_current_state->OnWindowSizeChanged(new_size);

    Render();
}

static bool OnLoadAssets() {
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

static void OnDestroy() {
    delete g_current_state;
}

bool App::Init(sge::RenderBackend backend, AppConfig config, int16_t world_width, int16_t world_height) {
    ZoneScoped;

    sge::Engine::SetLoadAssetsCallback(OnLoadAssets);
    sge::Engine::SetPreUpdateCallback(PreUpdate);
    sge::Engine::SetUpdateCallback(Update);
    sge::Engine::SetPostUpdateCallback(PostUpdate);
    sge::Engine::SetFixedUpdateCallback(FixedUpdate);
    sge::Engine::SetRenderCallback(Render);
    sge::Engine::SetPostRenderCallback(PostRender);
    sge::Engine::SetDestroyCallback(OnDestroy);
    sge::Engine::SetWindowResizeCallback(OnWindowResized);

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

    ParticleManager::Init();

    g_window_size.x = engine_config.window_settings.width;
    g_window_size.y = engine_config.window_settings.height;
    g_current_state = new InGameState();

    sge::Engine::ShowWindow();

    return true;
}

glm::uvec2 App::GetWindowResolution() {
    return g_window_size;
}

void App::Run() {
    sge::Engine::Run();
}

void App::Destroy() {
    GameRenderer::Terminate();
    ParticleManager::Terminate();
    sge::Engine::Destroy();
}