#ifndef ENGINE_HPP
#define ENGINE_HPP

#pragma once

#include <glm/vec2.hpp>

#include "types/backend.hpp"
#include "types/window_settings.hpp"

namespace Engine {
    using PreUpdateCallback = void (*)(void);
    using UpdateCallback = void (*)(void);
    using PostUpdateCallback = void (*)(void);
    using FixedUpdateCallback = void (*)(void);
    using RenderCallback = void (*)(void);
    using PostRenderCallback = void (*)(void);
    using DestroyCallback = void (*)(void);
    using LoadAssetsCallback = bool (*)(void);
    using WindowResizeCallback = void (*)(uint32_t width, uint32_t height, uint32_t scaled_width, uint32_t scaled_height);

    bool Init(RenderBackend backend, bool vsync, WindowSettings settings, glm::uvec2* viewport);
    void SetPreUpdateCallback(PreUpdateCallback);
    void SetUpdateCallback(UpdateCallback);
    void SetPostUpdateCallback(PostUpdateCallback);
    void SetFixedUpdateCallback(FixedUpdateCallback);
    void SetRenderCallback(RenderCallback);
    void SetPostRenderCallback(PostRenderCallback);
    void SetDestroyCallback(DestroyCallback);
    void SetWindowResizeCallback(WindowResizeCallback);
    void SetLoadAssetsCallback(LoadAssetsCallback);

    void SetWindowMinSize(uint32_t min_width, uint32_t min_height);
    void ShowWindow();
    void HideWindow();

    void ShowCursor();
    void HideCursor();

    void Run();
    void Destroy();
};

#endif