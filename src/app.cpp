#include "app.hpp"

#include <cstdlib>
#include <string>
#include <filesystem>

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
#include "state/menu/menu.hpp"
#include "world/autotile.hpp"
#include "state/base.hpp"
#include "state/ingame.hpp"
#include "state/testui.hpp"

#include "particles.hpp"
#include "assets.hpp"
#include "constants.hpp"

bool App::OnInit() {
    if (!InitRenderContext(m_config.backend)) {
        return false;
    }
    
    if (!Assets::Load(*GetRenderContext()))
        return false;

    if (!Assets::LoadFonts(*GetRenderContext()))
        return false;

    const std::vector<sge::ShaderDef> shader_defs = {
        sge::ShaderDef("TILE_SIZE", std::to_string(Constants::TILE_SIZE)),
        sge::ShaderDef("WALL_SIZE", std::to_string(Constants::WALL_SIZE)),
        sge::ShaderDef("DEF_SUBDIVISION", std::to_string(Constants::SUBDIVISION)),
        sge::ShaderDef("DEF_SOLID_DECAY", std::to_string(Constants::LightDecay(true))),
        sge::ShaderDef("DEF_AIR_DECAY", std::to_string(Constants::LightDecay(false))),
    };

    if (!Assets::LoadShaders(*GetRenderContext(), shader_defs))
        return false;

    sge::WindowSettings window_settings;
    window_settings.title = "TerrariaClone";
    window_settings.width = 1280;
    window_settings.height = 720;
    window_settings.cursor_mode = sge::CursorMode::Hidden;
    window_settings.samples = m_config.samples;
    window_settings.fullscreen = m_config.fullscreen;
    window_settings.vsync = m_config.vsync;
    window_settings.hidden = true;

    auto result = CreateWindow(window_settings);
    if (!result.has_value()) {
        SGE_LOG_ERROR("Couldn't create a window: {}", result.error());
        std::abort();
    }
    m_primary_window = result.value();

    namespace fs = std::filesystem;
    fs::create_directory(fs::current_path() / "worlds");

    sge::Time::SetFixedTimestepSeconds(Constants::FIXED_UPDATE_INTERVAL);

    InitTileRules();

    ParticleManager::Init();

    m_renderer = std::make_shared<sge::Renderer2D>(GetRenderContext());
    m_current_state = std::make_unique<MainMenuState>(m_renderer, m_config.samples);
    m_current_state->OnWindowSizeChanged(m_primary_window->GetSize());
    m_current_state->OnFramebufferSizeChanged(m_primary_window->GetContentSize());

    m_primary_window->ShowWindow();

    return true;
}

App::~App() {
    ParticleManager::Terminate();
}

void App::OnPreUpdate() {
    FrameTime::Update(sge::Time::DeltaSeconds());

    m_current_state->PreUpdate();
    m_current_state->OnPreFixedUpdate();
}

void App::OnFixedUpdate() {
    m_current_state->FixedUpdate();
}

void App::OnUpdate() {
    m_current_state->OnPostFixedUpdate();
    m_current_state->Update();
    m_current_state->PostUpdate();

    BaseState* new_state = m_current_state->GetNextState();
    if (new_state == nullptr) {
        Stop();
    } else if (new_state != m_current_state.get()) {
        m_current_state.reset(new_state);
        m_current_state->OnWindowSizeChanged(m_primary_window->GetSize());
        m_current_state->OnFramebufferSizeChanged(m_primary_window->GetContentSize());
    }
}

void App::OnRender(const std::shared_ptr<sge::GlfwWindow>& window) {
    m_current_state->Render(window);
}

void App::OnPostRender() {
    m_current_state->PostRender();
#if SGE_DEBUG_LAYER_ENABLED
    if (sge::Input::Pressed(sge::Key::C)) {
        LLGL::FrameProfile profile;
        m_renderer->GetRenderContext()->GetFrameProfile(&profile);
        SGE_LOG_DEBUG("Draw commands count: {}", profile.commandBufferRecord.drawCommands);
    }
#endif
}
