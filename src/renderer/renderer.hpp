#ifndef TERRARIA_RENDERER_HPP
#define TERRARIA_RENDERER_HPP

#pragma once

#include <LLGL/SwapChain.h>
#include <LLGL/RenderSystem.h>

#include "../types/sprite.hpp"
#include "../types/backend.hpp"
#include "../types/background_layer.hpp"
#include "../world/world.hpp"
#include "../assets.hpp"
#include "../particles.hpp"

#include "custom_surface.hpp"
#include "camera.h"

enum class RenderLayer : uint8_t {
    Main = 0,
    World = 1,
};

struct __attribute__((aligned(16))) ProjectionsUniform {
    glm::mat4 screen_projection_matrix;
    glm::mat4 view_projection_matrix;
    glm::mat4 nonscale_view_projection_matrix;
    glm::mat4 nonscale_projection_matrix;
    glm::mat4 transform_matrix;
    glm::vec2 camera_position;
    glm::vec2 window_size;
    float max_depth;
};

constexpr float MAX_Z = 1000.0f;

namespace Renderer {
    bool InitEngine(RenderBackend backend);
    bool Init(GLFWwindow* window, const LLGL::Extent2D& resolution, bool vsync, bool fullscreen);
    void RenderWorld();

    void Begin(const Camera& camera);
    void Render(const Camera& camera, const World& world);

    void DrawSprite(const Sprite& sprite, RenderLayer render_layer = RenderLayer::Main, int depth = -1);
    inline void DrawSprite(const Sprite& sprite, int depth) {
        DrawSprite(sprite, RenderLayer::Main, depth);
    }
    
    void DrawAtlasSprite(const TextureAtlasSprite& sprite, RenderLayer render_layer = RenderLayer::Main, int depth = -1);
    inline void DrawAtlasSprite(const TextureAtlasSprite& sprite, int depth) {
        DrawAtlasSprite(sprite, RenderLayer::Main, depth);
    }

    void DrawSpriteUI(const Sprite& sprite, int depth = -1);
    void DrawAtlasSpriteUI(const TextureAtlasSprite& sprite, int depth = -1);

    void DrawText(const char* text, uint32_t length, float size, const glm::vec2& position, const glm::vec3& color, FontKey font, bool is_ui = false, int depth = -1);

    inline void DrawText(const std::string& text, float size, const glm::vec2& position, const glm::vec3& color, FontKey font) {
        DrawText(text.c_str(), text.length(), size, position, color, font, false);
    }
    inline void DrawTextUi(const char* text, uint32_t length, float size, const glm::vec2& position, const glm::vec3& color, FontKey font, int depth = -1) {
        DrawText(text, length, size, position, color, font, true, depth);
    }
    inline void DrawTextUi(const std::string& text, float size, const glm::vec2& position, const glm::vec3& color, FontKey font, int depth = -1) {
        DrawText(text.c_str(), text.length(), size, position, color, font, true, depth);
    }

    inline void DrawChar(char ch, float size, const glm::vec2& position, const glm::vec3& color, FontKey font, int depth = -1) {
        DrawText(&ch, 1, size, position, color, font, false);
    }
    inline void DrawCharUi(char ch, float size, const glm::vec2& position, const glm::vec3& color, FontKey font, int depth = -1) {
        DrawText(&ch, 1, size, position, color, font, true);
    }

    void DrawBackground(const BackgroundLayer& layer);
    void DrawParticle(const glm::vec2& position, const glm::quat& rotation, float scale, Particle::Type type, uint8_t variant, int depth = -1);

#if DEBUG
    void PrintDebugInfo();
#endif

    void Terminate();

    [[nodiscard]] const LLGL::RenderSystemPtr& Context();
    [[nodiscard]] LLGL::SwapChain* SwapChain();
    [[nodiscard]] LLGL::CommandBuffer* CommandBuffer();
    [[nodiscard]] LLGL::CommandQueue* CommandQueue();
    [[nodiscard]] const std::shared_ptr<CustomSurface>& Surface();
    [[nodiscard]] LLGL::Buffer* GlobalUniformBuffer();
    [[nodiscard]] RenderBackend Backend();
    [[nodiscard]] uint32_t GetGlobalDepthIndex();
    [[nodiscard]] const LLGL::RenderPass* DefaultRenderPass();
};

#endif