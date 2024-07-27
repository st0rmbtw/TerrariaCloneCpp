#include <GLFW/glfw3.h>

#include "game.hpp"
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

static struct GameState {
    GLFWwindow *window;
    Camera camera;
    World world;
    Player player;
} g;

void update();
void render();

static void handle_keyboard_events(GLFWwindow* window, int key, int scancode, int action, int mods);
static void handle_mouse_button_events(GLFWwindow* window, int button, int action, int mods);
static void handle_mouse_scroll_events(GLFWwindow* window, double xoffset, double yoffset);
static void handle_cursor_pos_events(GLFWwindow* window, double xpos, double ypos);
static void handle_window_resize_events(GLFWwindow* window, int width, int height);

GLFWwindow* create_window(LLGL::Extent2D size) {
    glfwWindowHint(GLFW_FOCUSED, 1);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWmonitor* primary_monitor = nullptr;
#if FULLSCREEN
    primary_monitor = glfwGetPrimaryMonitor();
#endif

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

#ifdef BACKEND_OPENGL
    glfwSwapInterval(VSYNC);
#endif

    return window;
}

bool Game::Init() {
    if (!glfwInit()) {
        LOG_ERROR("Couldn't initialize GLFW: %s", glfwGetErrorString());
        return false;
    }

#if FULLSCREEN
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    const auto window_size = LLGL::Extent2D(mode->width, mode->height);
#else
    const auto window_size = LLGL::Extent2D(1280, 720);
#endif

    const LLGL::Display* display = LLGL::Display::GetPrimary();
    const std::uint32_t resScale = (display != nullptr ? static_cast<std::uint32_t>(display->GetScale()) : 1u);
    const auto resolution = LLGL::Extent2D(window_size.width * resScale, window_size.height * resScale);

    GLFWwindow *window = create_window(window_size);
    if (window == nullptr) return false;

    LLGL::Log::RegisterCallbackStd();

    if (!Renderer::InitEngine()) return false;
    if (!Assets::Load()) return false;
    if (!Assets::InitSamplers()) return false;

    init_tile_rules();

    g.window = window;
    g.camera.set_viewport({resolution.width, resolution.height});

    g.world.generate(200, 500, 0);

    std::vector<ShaderDef> shader_defs = {
        // ShaderDef("DEF_SUBDIVISION", std::to_string(config::SUBDIVISION)),
        ShaderDef("DEF_WORLD_WIDTH", std::to_string(g.world.area().width())),
        ShaderDef("DEF_WORLD_HEIGHT", std::to_string(g.world.area().height())),
        ShaderDef("DEF_CHUNK_WIDTH", std::to_string(RENDER_CHUNK_SIZE_U)),
        ShaderDef("DEF_CHUNK_HEIGHT", std::to_string(RENDER_CHUNK_SIZE_U)),

        ShaderDef("DEF_TILE_SIZE", std::to_string(Constants::TILE_SIZE)),
        ShaderDef("DEF_WALL_SIZE", std::to_string(Constants::WALL_SIZE)),

        ShaderDef("DEF_TILE_TEXTURE_WIDTH", std::to_string(Constants::MAX_TILE_TEXTURE_WIDTH)),
        ShaderDef("DEF_TILE_TEXTURE_HEIGHT", std::to_string(Constants::MAX_TILE_TEXTURE_HEIGHT)),
        ShaderDef("DEF_TILE_TEXTURE_PADDING", std::to_string(Constants::TILE_TEXTURE_PADDING)),

        ShaderDef("DEF_WALL_TEXTURE_WIDTH", std::to_string(Constants::MAX_WALL_TEXTURE_WIDTH)),
        ShaderDef("DEF_WALL_TEXTURE_HEIGHT", std::to_string(Constants::MAX_WALL_TEXTURE_HEIGHT)),
        ShaderDef("DEF_WALL_TEXTURE_PADDING", std::to_string(Constants::WALL_TEXTURE_PADDING)),
    };

    if (!Assets::LoadShaders(shader_defs)) return false;
    
    if (!Renderer::Init(window, resolution)) return false;

    g.player.inventory().set_item(0, ITEM_COPPER_AXE);
    g.player.inventory().set_item(1, ITEM_COPPER_PICKAXE);
    g.player.inventory().set_item(2, ITEM_COPPER_HAMMER);
    g.player.inventory().set_item(3, ITEM_DIRT_BLOCK.with_max_stack());
    g.player.inventory().set_item(4, ITEM_STONE_BLOCK.with_max_stack());
    g.player.inventory().set_item(5, ITEM_WOOD_BLOCK.with_max_stack());

    return true;
}

void Game::Run() {
    double prev_tick = glfwGetTime();
    
    while (Renderer::Surface()->ProcessEvents()) {
        double current_tick = glfwGetTime();
        const double delta_time = (current_tick - prev_tick);
        prev_tick = current_tick;

        delta_time_t dt(delta_time);
        Time::advance_by(dt);

        update();
        render();

        KeyboardInput::clear();
        MouseInput::clear();
    }
}

void Game::Destroy() {
    g.world.clear_chunks();
    Renderer::Terminate();
    glfwTerminate();
}

void update() {
    if (KeyboardInput::Pressed(Key::W)) {
        glm::vec2 position = g.camera.position();
        position.y -= 1000.0f * Time::delta_seconds();
        g.camera.set_position(position);
    }

    if (KeyboardInput::Pressed(Key::S)) {
        glm::vec2 position = g.camera.position();
        position.y += 1000.0f * Time::delta_seconds();
        g.camera.set_position(position);
    }

    if (KeyboardInput::Pressed(Key::A)) {
        glm::vec2 position = g.camera.position();
        position.x -= 1000.0f * Time::delta_seconds();
        g.camera.set_position(position);
    }

    if (KeyboardInput::Pressed(Key::D)) {
        glm::vec2 position = g.camera.position();
        position.x += 1000.0f * Time::delta_seconds();
        g.camera.set_position(position);
    }

    g.camera.update();
    g.world.update(g.camera);

    UI::Update(g.player.inventory());
}

void render() {
    Renderer::Begin(g.camera);

    Renderer::RenderWorld(g.world);

    UI::Render(g.camera, g.player.inventory());

    Renderer::Render();
}

static void handle_keyboard_events(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        KeyboardInput::press(static_cast<Key>(key));
    } else if (action == GLFW_RELEASE) {
        KeyboardInput::release(static_cast<Key>(key));
    }
}

static void handle_mouse_button_events(GLFWwindow* window, int button, int action, int mods) {
    if (action == GLFW_PRESS) {
        MouseInput::press(static_cast<MouseButton>(button));
    } else if (action == GLFW_RELEASE) {
        MouseInput::release(static_cast<MouseButton>(button));
    }
}

static void handle_mouse_scroll_events(GLFWwindow* window, double xoffset, double yoffset) {
    MouseInput::push_scroll_event(yoffset);
}

static void handle_cursor_pos_events(GLFWwindow* window, double xpos, double ypos) {
    MouseInput::set_screen_position(glm::vec2(xpos, ypos));
}

static void handle_window_resize_events(GLFWwindow* window, int width, int height) {
    const LLGL::Display* display = LLGL::Display::GetPrimary();
    const std::uint32_t resScale = (display != nullptr ? static_cast<std::uint32_t>(display->GetScale()) : 1u);

    const auto new_size = LLGL::Extent2D(width * resScale, height * resScale);
    Renderer::SwapChain()->ResizeBuffers(new_size);

    g.camera.set_viewport({new_size.width, new_size.height});

    render();
}