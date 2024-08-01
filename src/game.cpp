#include <GLFW/glfw3.h>

#include "game.hpp"
#include "constants.hpp"
#include "renderer/camera.h"
#include "renderer/renderer.hpp"
#include "time/time.hpp"
#include "assets.hpp"
#include "log.hpp"
#include "utils.hpp"
#include "input.hpp"
#include "ui.hpp"
#include "world/autotile.hpp"
#include "player/player.hpp"
#include "world/particles.hpp"

static struct GameState {
    Camera camera;
    World world;
    Player player;
    GLFWwindow *window = nullptr;
    bool free_camera = false;
    bool minimized = false;
} g;

void pre_update();
void fixed_update();
void update();
void render();
void post_render();

glm::vec2 camera_follow_player();
#if DEBUG
glm::vec2 camera_free();
#endif

static void handle_keyboard_events(GLFWwindow* window, int key, int scancode, int action, int mods);
static void handle_mouse_button_events(GLFWwindow* window, int button, int action, int mods);
static void handle_mouse_scroll_events(GLFWwindow* window, double xoffset, double yoffset);
static void handle_cursor_pos_events(GLFWwindow* window, double xpos, double ypos);
static void handle_window_resize_events(GLFWwindow* window, int width, int height);
static void handle_window_iconify_callback(GLFWwindow* window, int iconified);

GLFWwindow* create_window(LLGL::Extent2D size, bool fullscreen) {
    glfwWindowHint(GLFW_FOCUSED, 1);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWmonitor* primary_monitor = fullscreen ? glfwGetPrimaryMonitor() : nullptr;

    GLFWwindow *window = glfwCreateWindow(size.width, size.height, "AAA", primary_monitor, nullptr);
    if (window == nullptr) {
        LOG_ERROR("Couldn't create a window: %s", glfwGetErrorString());
        glfwTerminate();
        return nullptr;
    }

    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, handle_keyboard_events);
    glfwSetMouseButtonCallback(window, handle_mouse_button_events);
    glfwSetScrollCallback(window, handle_mouse_scroll_events);
    glfwSetCursorPosCallback(window, handle_cursor_pos_events);
    glfwSetWindowSizeCallback(window, handle_window_resize_events);
    glfwSetWindowIconifyCallback(window, handle_window_iconify_callback);

    return window;
}

bool Game::Init(RenderBackend backend, GameConfig config) {
    if (!glfwInit()) {
        LOG_ERROR("Couldn't initialize GLFW: %s", glfwGetErrorString());
        return false;
    }

    auto window_size = LLGL::Extent2D(1280, 720);
    if (config.fullscreen) {
        const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        window_size = LLGL::Extent2D(mode->width, mode->height);
    }

    const LLGL::Display* display = LLGL::Display::GetPrimary();
    const std::uint32_t resScale = (display != nullptr ? static_cast<std::uint32_t>(display->GetScale()) : 1u);
    const auto resolution = LLGL::Extent2D(window_size.width * resScale, window_size.height * resScale);

    LLGL::Log::RegisterCallbackStd();

    if (!Renderer::InitEngine(backend)) return false;
    if (!Assets::Load()) return false;
    if (!Assets::InitSamplers()) return false;

    init_tile_rules();

    g.camera.set_viewport({resolution.width, resolution.height});
    g.camera.set_zoom(1.0f);

    g.world.generate(200, 500, 0);

    const std::vector<ShaderDef> shader_defs = {
        // ShaderDef("DEF_SUBDIVISION", std::to_string(config::SUBDIVISION)),
        ShaderDef("WORLD_WIDTH", std::to_string(g.world.area().width())),
        ShaderDef("WORLD_HEIGHT", std::to_string(g.world.area().height())),
        ShaderDef("CHUNK_WIDTH", std::to_string(RENDER_CHUNK_SIZE_U)),
        ShaderDef("CHUNK_HEIGHT", std::to_string(RENDER_CHUNK_SIZE_U)),

        ShaderDef("TILE_SIZE", std::to_string(Constants::TILE_SIZE)),
        ShaderDef("WALL_SIZE", std::to_string(Constants::WALL_SIZE)),

        ShaderDef("TILE_TEXTURE_WIDTH", std::to_string(Constants::MAX_TILE_TEXTURE_WIDTH)),
        ShaderDef("TILE_TEXTURE_HEIGHT", std::to_string(Constants::MAX_TILE_TEXTURE_HEIGHT)),
        ShaderDef("TILE_TEXTURE_PADDING", std::to_string(Constants::TILE_TEXTURE_PADDING)),

        ShaderDef("WALL_TEXTURE_WIDTH", std::to_string(Constants::MAX_WALL_TEXTURE_WIDTH)),
        ShaderDef("WALL_TEXTURE_HEIGHT", std::to_string(Constants::MAX_WALL_TEXTURE_HEIGHT)),
        ShaderDef("WALL_TEXTURE_PADDING", std::to_string(Constants::WALL_TEXTURE_PADDING)),
    };

    if (!Assets::LoadShaders(shader_defs)) return false;

    GLFWwindow *window = create_window(window_size, config.fullscreen);
    if (window == nullptr) return false;

    g.window = window;
    
    if (!Renderer::Init(window, resolution, config.vsync, config.fullscreen)) return false;

    ParticleManager::Init();

    g.player.init();
    g.player.inventory().set_item(0, ITEM_COPPER_AXE);
    g.player.inventory().set_item(1, ITEM_COPPER_PICKAXE);
    g.player.inventory().set_item(2, ITEM_COPPER_HAMMER);
    g.player.inventory().set_item(3, ITEM_DIRT_BLOCK.with_max_stack());
    g.player.inventory().set_item(4, ITEM_STONE_BLOCK.with_max_stack());
    g.player.inventory().set_item(5, ITEM_WOOD_BLOCK.with_max_stack());
    g.player.set_position(g.world, glm::vec2(g.world.spawn_point()) * Constants::TILE_SIZE);

    return true;
}

void Game::Run() {
    double prev_tick = glfwGetTime();

    float fixed_timer = 0;
    
    while (Renderer::Surface()->ProcessEvents()) {
        const double current_tick = glfwGetTime();
        const double delta_time = (current_tick - prev_tick);
        prev_tick = current_tick;

        const delta_time_t dt(delta_time);
        Time::advance_by(dt);

        pre_update();

        fixed_timer += delta_time;
        while (fixed_timer > Constants::FIXED_UPDATE_INTERVAL) {
            Time::fixed_advance_by(delta_time_t(Constants::FIXED_UPDATE_INTERVAL));
            fixed_update();
            fixed_timer -= Constants::FIXED_UPDATE_INTERVAL;
        }

        update();
        
        if (!g.minimized) {
            render();
            post_render();
        }

        Input::Clear();
    }
}

void Game::Destroy() {
    Renderer::CommandQueue()->WaitIdle();

    g.world.destroy_chunks();
    
    Renderer::Terminate();
    glfwTerminate();
}

void pre_update() {
    ParticleManager::DeleteExpired();

    g.player.pre_update();

    if (Input::JustPressed(Key::F)) g.free_camera = !g.free_camera;
}

void update() {
#if DEBUG
    const glm::vec2 position = g.free_camera ? camera_free() : camera_follow_player();
#else
    const glm::vec2 position = camera_follow_player();
#endif

    g.camera.set_position(position);

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

    g.camera.update();
    g.world.update(g.camera);
    g.player.update(g.camera, g.world);

    UI::Update(g.player.inventory());

    if (Input::JustPressed(Key::K)) {
        // for (int i = 0; i < 1; i++) {
        // const glm::vec2 position = g.camera.screen_to_world(Input::MouseScreenPosition());
        // const glm::vec2 velocity = glm::diskRand(1.0f) * 1.5f;
        const glm::vec2 velocity = glm::vec2(0.0f);
        const glm::vec2 position = glm::vec2(g.player.position()) - glm::vec2(0.0f, 100.0f);

        ParticleManager::SpawnParticle(
            ParticleBuilder::create(Particle::Type::Grass, position, velocity, 1.5f)
                .with_rotation_speed(glm::pi<float>() / 12.0f)
                .with_scale(0.5f)
        );
        // }
    }
}

void fixed_update() {
    ParticleManager::Update();

#if DEBUG
    bool handle_input = !g.free_camera;
#else
    bool handle_input = true;
#endif

    g.player.fixed_update(g.world, handle_input);
}

void render() {
    Renderer::Begin(g.camera);

    Renderer::RenderWorld(g.world);

    g.player.render();

    ParticleManager::Render();

    UI::Render(g.camera, g.player.inventory());

    Renderer::Render();
}

void post_render() {
    std::deque<RenderChunk>& chunks = g.world.chunks_to_destroy();
    if (!chunks.empty()) {
        Renderer::CommandQueue()->WaitIdle();
        while (!chunks.empty()) {
            chunks.back().destroy();
            chunks.pop_back();
        }
    }

#if DEBUG
    if (Input::Pressed(Key::C)) {
        Renderer::PrintDebugInfo();
    }
#endif
}

glm::vec2 camera_follow_player() {
    glm::vec2 position = g.player.position();

    const math::Rect area = g.world.playable_area() * Constants::TILE_SIZE;
    const math::Rect& camera_area = g.camera.get_projection_area();
    
    const float left = camera_area.min.x;
    const float right = camera_area.max.x;

    const float top = camera_area.min.y;
    const float bottom = camera_area.max.y;

    if (position.x + left < area.min.x) position.x = area.min.x - left;
    if (position.x + right > area.max.x) position.x = area.max.x - right;

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

    return position;
}
#endif

static void handle_keyboard_events(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        Input::Press(static_cast<Key>(key));
    } else if (action == GLFW_RELEASE) {
        Input::Release(static_cast<Key>(key));
    }
}

static void handle_mouse_button_events(GLFWwindow* window, int button, int action, int mods) {
    if (action == GLFW_PRESS) {
        Input::press(static_cast<MouseButton>(button));
    } else if (action == GLFW_RELEASE) {
        Input::release(static_cast<MouseButton>(button));
    }
}

static void handle_mouse_scroll_events(GLFWwindow* window, double xoffset, double yoffset) {
    Input::PushMouseScrollEvent(yoffset);
}

static void handle_cursor_pos_events(GLFWwindow* window, double xpos, double ypos) {
    Input::SetMouseScreenPosition(glm::vec2(xpos, ypos));
}

static void handle_window_resize_events(GLFWwindow* window, int width, int height) {
    if (width <= 0 || height <= 0) {
        g.minimized = true;
        return;
    } else {
        g.minimized = false;
    }

    const LLGL::Display* display = LLGL::Display::GetPrimary();
    const std::uint32_t resScale = (display != nullptr ? static_cast<std::uint32_t>(display->GetScale()) : 1u);

    const auto new_size = LLGL::Extent2D(width * resScale, height * resScale);

    Renderer::SwapChain()->ResizeBuffers(new_size);

    g.camera.set_viewport({new_size.width, new_size.height});

    render();
}

static void handle_window_iconify_callback(GLFWwindow* window, int iconified) {
    g.minimized = iconified;
}