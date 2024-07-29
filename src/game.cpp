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
    bool free_camera = false;
} g;

void pre_update();
void fixed_update(delta_time_t delta_time);
void update();
void render();

glm::vec2 camera_follow_player();
#if DEBUG
glm::vec2 camera_free();
#endif

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

    LLGL::Log::RegisterCallbackStd();

    if (!Renderer::InitEngine()) return false;
    if (!Assets::Load()) return false;
    if (!Assets::InitSamplers()) return false;

    init_tile_rules();

    g.camera.set_viewport({resolution.width, resolution.height});

    g.world.generate(200, 500, 0);

    std::vector<ShaderDef> shader_defs = {
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

    GLFWwindow *window = create_window(window_size);
    if (window == nullptr) return false;

    g.window = window;
    
    if (!Renderer::Init(window, resolution)) return false;

    g.player.init();
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

    float fixed_timer = 0;
    
    while (Renderer::Surface()->ProcessEvents()) {
        double current_tick = glfwGetTime();
        const double delta_time = (current_tick - prev_tick);
        prev_tick = current_tick;

        delta_time_t dt(delta_time);
        Time::advance_by(dt);

        pre_update();

        fixed_timer += delta_time;
        while (fixed_timer > Constants::FIXED_UPDATE_INTERVAL) {
            fixed_update(delta_time_t(Constants::FIXED_UPDATE_INTERVAL));
            fixed_timer -= Constants::FIXED_UPDATE_INTERVAL;
        }

        update();
        render();

        KeyboardInput::Clear();
        MouseInput::Clear();
    }
}

void Game::Destroy() {
    g.world.clear_chunks();
    Renderer::Terminate();
    glfwTerminate();
}

void pre_update() {
    g.player.pre_update();

    if (KeyboardInput::JustPressed(Key::F)) g.free_camera = !g.free_camera;
}

void update() {
#if DEBUG
    if (g.free_camera) {
        g.camera.set_position(camera_free());
    }
#endif

    g.camera.update();
    g.world.update(g.camera);
    g.player.update(g.camera, g.world);

    UI::Update(g.player.inventory());
}

void fixed_update(delta_time_t delta_time) {
    const float dt = Time::delta_seconds();

    // if (m_game_state == GameState::InGame) {
        // ParticleManager::fixed_update();

#if DEBUG
        if (g.free_camera) {
            g.player.fixed_update(delta_time, g.world, false);
        } else {
#endif
            g.player.fixed_update(delta_time, g.world, true);
            g.camera.set_position(camera_follow_player());
#if DEBUG
        }
#endif
    // }
}

void render() {
    Renderer::Begin(g.camera);

    Renderer::RenderWorld(g.world);

    g.player.render();

    UI::Render(g.camera, g.player.inventory());

    Renderer::Render();
}

glm::vec2 camera_follow_player() {
    glm::vec2 position = g.player.position();

    const float world_min_x = g.world.playable_area().min.x * Constants::TILE_SIZE;
    const float world_max_x = g.world.playable_area().max.x * Constants::TILE_SIZE;
    const float world_min_y = g.world.playable_area().min.y * Constants::TILE_SIZE;
    const float world_max_y = g.world.playable_area().max.y * Constants::TILE_SIZE;

    const math::Rect& camera_area = g.camera.get_projection_area();
    
    const float left = camera_area.min.x;
    const float right = camera_area.max.x;

    const float top = camera_area.min.y;
    const float bottom = camera_area.max.y;

    if (position.x + left < world_min_x) position.x = world_min_x - left;
    if (position.x + right > world_max_x) position.x = world_max_x - right;

    if (position.y + top < world_min_y) position.y = world_min_y - top;
    if (position.y + bottom > world_max_y) position.y = world_max_y - bottom;

    return position;
}

#if DEBUG
glm::vec2 camera_free() {
    float dt = Time::delta_seconds();
    glm::vec2 position = g.camera.position();

    float speed = 2000.0f;

    if (KeyboardInput::Pressed(Key::LeftShift)) {
        speed *= 2.0f;
    }

    if (KeyboardInput::Pressed(Key::LeftAlt)) {
        speed /= 5.0f;
    }

    if (KeyboardInput::Pressed(Key::A)) {
        position.x -= speed * dt;
    }

    if (KeyboardInput::Pressed(Key::D)) {
        position.x += speed * dt;
    }

    if (KeyboardInput::Pressed(Key::W)) {
        position.y -= speed * dt;
    }

    if (KeyboardInput::Pressed(Key::S)) {
        position.y += speed * dt;
    }

    return position;
}
#endif

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