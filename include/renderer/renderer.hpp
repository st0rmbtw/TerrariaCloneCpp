#ifndef TERRARIA_RENDERER_HPP
#define TERRARIA_RENDERER_HPP

#pragma once

#include <LLGL/LLGL.h>
#include "renderer/custom_surface.hpp"
#include "renderer/camera.h"
#include "types/sprite.hpp"
#include "types/backend.hpp"
#include "world/world.hpp"

enum class RenderLayer : uint8_t {
    Main = 0,
    World = 1,
    UI = 2
};

namespace Renderer {
    bool InitEngine(RenderBackend backend);
    bool Init(GLFWwindow* window, const LLGL::Extent2D& resolution, bool vsync, bool fullscreen);
    void RenderWorld(const World& world);

    void Begin(const Camera& camera);
    void Render();

    void DrawSprite(const Sprite& sprite, RenderLayer render_layer = RenderLayer::Main);
    void DrawAtlasSprite(const TextureAtlasSprite& sprite, RenderLayer render_layer = RenderLayer::Main);
    void FlushSpriteBatch();

#if DEBUG
    void PrintDebugInfo();
#endif

    void Terminate();

    const LLGL::RenderSystemPtr& Context();
    LLGL::SwapChain* SwapChain();
    LLGL::CommandBuffer* CommandBuffer();
    LLGL::CommandQueue* CommandQueue();
    const std::shared_ptr<CustomSurface>& Surface();
    RenderBackend Backend();
};

#endif