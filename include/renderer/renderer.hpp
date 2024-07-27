#pragma once

#ifndef TERRARIA_RENDERER_HPP
#define TERRARIA_RENDERER_HPP

#include <LLGL/LLGL.h>
#include "renderer/custom_surface.hpp"
#include "renderer/defines.h"
#include "renderer/camera.h"
#include "types/sprite.hpp"
#include "world/world.hpp"

enum class RenderLayer : uint8_t {
    Main = 0,
    World = 1,
    UI = 2
};

namespace Renderer {
    bool InitEngine();
    bool Init(GLFWwindow* window, const LLGL::Extent2D& resolution);
    void RenderWorld(const World& world);

    void Begin(const Camera& camera);
    void Render();

    void DrawSprite(const Sprite& sprite, RenderLayer render_layer = RenderLayer::Main);
    void DrawAtlasSprite(const TextureAtlasSprite& sprite, RenderLayer render_layer = RenderLayer::Main);
    void FlushSpriteBatch(void);

    void Terminate(void);

    const LLGL::RenderSystemPtr& Context(void);
    LLGL::SwapChain* SwapChain(void);
    LLGL::CommandBuffer* CommandBuffer(void);
    const std::shared_ptr<CustomSurface>& Surface(void);
};

#endif