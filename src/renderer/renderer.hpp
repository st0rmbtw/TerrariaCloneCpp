#ifndef TERRARIA_RENDERER_HPP
#define TERRARIA_RENDERER_HPP

#pragma once

#include <LLGL/SwapChain.h>
#include <LLGL/RenderSystem.h>

#include "../types/sprite.hpp"
#include "../types/backend.hpp"
#include "../world/world.hpp"
#include "../assets.hpp"

#include "custom_surface.hpp"
#include "camera.h"

enum class RenderLayer : uint8_t {
    Main = 0,
    World = 1,
    UI = 2
};

struct ProjectionsUniform {
    glm::mat4 screen_projection_matrix;
    glm::mat4 view_projection_matrix;
};

namespace Renderer {
    bool InitEngine(RenderBackend backend);
    bool Init(GLFWwindow* window, const LLGL::Extent2D& resolution, bool vsync, bool fullscreen);
    void RenderWorld(const World& world);

    void Begin(const Camera& camera);
    void Render();

    void DrawSprite(const Sprite& sprite, RenderLayer render_layer = RenderLayer::Main);
    void DrawAtlasSprite(const TextureAtlasSprite& sprite, RenderLayer render_layer = RenderLayer::Main);
    void DrawText(const char* text, uint32_t length, float size, const glm::vec2& position, const glm::vec3& color, FontKey font, RenderLayer render_layer = RenderLayer::Main);
    
    inline void DrawText(const std::string& text, float size, const glm::vec2& position, const glm::vec3& color, FontKey font, RenderLayer render_layer = RenderLayer::Main) {
        DrawText(text.c_str(), text.length(), size, position, color, font, render_layer);
    }
    inline void DrawTextUi(const char* text, uint32_t length, float size, const glm::vec2& position, const glm::vec3& color, FontKey font) {
        DrawText(text, length, size, position, color, font, RenderLayer::UI);
    }
    inline void DrawTextUi(const std::string& text, float size, const glm::vec2& position, const glm::vec3& color, FontKey font) {
        DrawText(text.c_str(), text.length(), size, position, color, font, RenderLayer::UI);
    }
    inline void DrawChar(char ch, float size, const glm::vec2& position, const glm::vec3& color, FontKey font, RenderLayer render_layer = RenderLayer::Main) {
        DrawText(&ch, 1, size, position, color, font, render_layer);
    }
    inline void DrawCharUi(char ch, float size, const glm::vec2& position, const glm::vec3& color, FontKey font) {
        DrawText(&ch, 1, size, position, color, font, RenderLayer::UI);
    }

#if DEBUG
    void PrintDebugInfo();
#endif

    void Terminate();

    const LLGL::RenderSystemPtr& Context();
    LLGL::SwapChain* SwapChain();
    LLGL::CommandBuffer* CommandBuffer();
    LLGL::CommandQueue* CommandQueue();
    const std::shared_ptr<CustomSurface>& Surface();
    LLGL::Buffer* ProjectionsUniformBuffer();
    RenderBackend Backend();
};

#endif