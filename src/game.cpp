#include <GLFW/glfw3.h>

#include "game.hpp"
#include "renderer/camera.h"
#include "renderer/renderer.hpp"
#include "assets.hpp"
#include "log.hpp"
#include "utils.hpp"
#include "input.hpp"
#include "ui.hpp"

static struct GameState {
    float delta_time;
    GLFWwindow *window;
    Camera camera;
} g;

void update();
void render();

static void handle_keyboard_events(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        KeyboardInput::press(static_cast<Key>(key));
    } else if (action == GLFW_RELEASE) {
        KeyboardInput::release(static_cast<Key>(key));
    }
}

void handle_mouse_button_events(GLFWwindow* window, int button, int action, int mods) {
    if (action == GLFW_PRESS) {
        MouseInput::press(static_cast<MouseButton>(button));
    } else if (action == GLFW_RELEASE) {
        MouseInput::release(static_cast<MouseButton>(button));
    }
}

void handle_mouse_scroll_events(GLFWwindow* window, double xoffset, double yoffset) {
    MouseInput::push_scroll_event(yoffset);
}

void handle_cursor_pos_events(GLFWwindow* window, double xpos, double ypos) {
    MouseInput::set_screen_position(glm::vec2(xpos, ypos));
}

void handle_window_resize_events(GLFWwindow* window, int width, int height) {
    const LLGL::Display* display = LLGL::Display::GetPrimary();
    const std::uint32_t resScale = (display != nullptr ? static_cast<std::uint32_t>(display->GetScale()) : 1u);

    const auto new_size = LLGL::Extent2D(width * resScale, height * resScale);
    Renderer::SwapChain()->ResizeBuffers(new_size);

    g.camera.set_viewport({new_size.width, new_size.height});

    render();
}

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

    if (!Renderer::Init(window, resolution)) return false;
    if (!Assets::Load()) return false;

    g.window = window;
    g.camera.set_viewport({resolution.width, resolution.height});

    return true;
}

void Game::Run() {
    double prev_tick = glfwGetTime();
    
    while (Renderer::Surface()->ProcessEvents()) {
        double current_tick = glfwGetTime();
        const double delta_time = (current_tick - prev_tick);
        prev_tick = current_tick;

        g.delta_time = delta_time;

        update();
        render();

        KeyboardInput::clear();
        MouseInput::clear();
    }
}

void update() {
    if (KeyboardInput::Pressed(Key::W)) {
        glm::vec2 position = g.camera.position();
        position.y -= 1000.0f * g.delta_time;
        g.camera.set_position(position);
    }

    if (KeyboardInput::Pressed(Key::S)) {
        glm::vec2 position = g.camera.position();
        position.y += 1000.0f * g.delta_time;
        g.camera.set_position(position);
    }

    if (KeyboardInput::Pressed(Key::A)) {
        glm::vec2 position = g.camera.position();
        position.x -= 1000.0f * g.delta_time;
        g.camera.set_position(position);
    }

    if (KeyboardInput::Pressed(Key::D)) {
        glm::vec2 position = g.camera.position();
        position.x += 1000.0f * g.delta_time;
        g.camera.set_position(position);
    }

    g.camera.update();
}

void render() {
    Renderer::Begin(g.camera);

    for (int i = 0; i < 6; i++) {
        Sprite sprite;

        const float x = (i % 3) * 100.0f + 10.0f * (i % 3) - 100.0f;
        const float y = (i / 3) * 100.0f + 10.0f * (i / 3) - 100.0f;

        sprite.set_position(glm::vec3(x, y, 1.0f));
        sprite.set_color(glm::vec3((i / 5.0f), (i / 5.0f), 1.0f));
        sprite.set_custom_size(glm::vec2(100.0f));

        Renderer::DrawSprite(sprite);
    }

    UI::Render(g.camera);

    Renderer::Render();
}

void Game::Destroy() {
    Assets::Unload();
    Renderer::Terminate();
    glfwTerminate();
}