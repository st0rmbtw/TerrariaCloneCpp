#pragma once

#ifndef APP_HPP_
#define APP_HPP_

#include <SGE/types/backend.hpp>
#include <SGE/engine.hpp>
#include <SGE/renderer/renderer.hpp>
#include <glm/vec2.hpp>

#include "state/base.hpp"

struct AppConfig {
    bool vsync = false;
    bool fullscreen = false;
    uint8_t samples = 1;
    sge::RenderBackend backend;
};


class App final : public sge::IEngine {
public:
    App(AppConfig config) : m_config(config) {
    }
    ~App();
protected:
    bool OnInit() override;
    void OnFixedUpdate() override;
    void OnPreUpdate() override;
    void OnUpdate() override;
    void OnRender(const std::shared_ptr<sge::GlfwWindow>& window) override;
    void OnPostRender() override;

    void OnWindowResized(const std::shared_ptr<sge::GlfwWindow>&, int width, int height) override {
        m_current_state->OnWindowSizeChanged(LLGL::Extent2D(width, height));
    }
    void OnFramebufferResize(const std::shared_ptr<sge::GlfwWindow>&, int width, int height) override {
        m_current_state->OnFramebufferSizeChanged(LLGL::Extent2D(width, height));
    }

private:
    std::unique_ptr<BaseState> m_current_state;
    std::shared_ptr<sge::Renderer2D> m_renderer;
    std::shared_ptr<sge::GlfwWindow> m_primary_window;
    AppConfig m_config;
};

#endif
