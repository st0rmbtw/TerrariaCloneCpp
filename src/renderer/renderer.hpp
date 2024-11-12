#ifndef TERRARIA_RENDERER_HPP
#define TERRARIA_RENDERER_HPP

#pragma once

#include <LLGL/SwapChain.h>
#include <LLGL/RenderSystem.h>

#include "../types/sprite.hpp"
#include "../types/backend.hpp"
#include "../types/background_layer.hpp"
#include "../types/render_layer.hpp"
#include "../assets.hpp"
#include "../particles.hpp"

#include "../world/world_data.hpp"
#include "../world/chunk_manager.hpp"

#include "custom_surface.hpp"
#include "camera.h"

struct __attribute__((aligned(16))) ProjectionsUniform {
    glm::mat4 screen_projection_matrix;
    glm::mat4 view_projection_matrix;
    glm::mat4 nonscale_view_projection_matrix;
    glm::mat4 nonscale_projection_matrix;
    glm::mat4 transform_matrix;
    glm::mat4 inv_view_proj_matrix;
    glm::vec2 camera_position;
    glm::vec2 window_size;
    float max_depth;
    float max_world_depth;
};

namespace Renderer {
    bool InitEngine(RenderBackend backend);
    bool Init(GLFWwindow* window, const LLGL::Extent2D& resolution, bool vsync, bool fullscreen);
    void InitWorldRenderer(const WorldData& world);
    void ResizeTextures(LLGL::Extent2D resolution);

    void Begin(const Camera& camera, WorldData& world);
    void Render(const Camera& camera, const ChunkManager& chunk_manager);

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

    void DrawText(const char* text, uint32_t length, float size, const glm::vec2& position, const glm::vec3& color, FontAsset font, bool is_ui = false, int depth = -1);

    inline void DrawText(const std::string& text, float size, const glm::vec2& position, const glm::vec3& color, FontAsset font) {
        DrawText(text.c_str(), text.length(), size, position, color, font, false);
    }
    inline void DrawTextUi(const char* text, uint32_t length, float size, const glm::vec2& position, const glm::vec3& color, FontAsset font, int depth = -1) {
        DrawText(text, length, size, position, color, font, true, depth);
    }
    inline void DrawTextUi(const std::string& text, float size, const glm::vec2& position, const glm::vec3& color, FontAsset font, int depth = -1) {
        DrawText(text.c_str(), text.length(), size, position, color, font, true, depth);
    }

    inline void DrawChar(char ch, float size, const glm::vec2& position, const glm::vec3& color, FontAsset font, int depth = -1) {
        DrawText(&ch, 1, size, position, color, font, false, depth);
    }
    inline void DrawCharUi(char ch, float size, const glm::vec2& position, const glm::vec3& color, FontAsset font, int depth = -1) {
        DrawText(&ch, 1, size, position, color, font, true, depth);
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
    [[nodiscard]] uint32_t GetMainDepthIndex();
    [[nodiscard]] uint32_t GetWorldDepthIndex();
    [[nodiscard]] uint32_t GetUiDepthIndex();
    [[nodiscard]] const LLGL::RenderPass* DefaultRenderPass();
    [[nodiscard]] LLGL::Buffer* ChunkVertexBuffer();
};

#endif