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
    App(AppConfig config, int16_t world_width, int16_t world_height) : m_config(config) {
    }

    bool Init() override;

    ~App();
protected:
    void OnFixedUpdate() override;
    void OnPreUpdate() override;
    void OnPostUpdate() override;
    void OnUpdate() override;
    void OnRender(const std::shared_ptr<sge::GlfwWindow> &window) override;
    void OnPostRender(const std::shared_ptr<sge::GlfwWindow> &window) override;

    void OnWindowResized(const std::shared_ptr<sge::GlfwWindow> &window, int width, int height) override;
    void OnFramebufferResize(const std::shared_ptr<sge::GlfwWindow> &window, int width, int height) override;

    void OnWindowDestroy(sge::GlfwWindow& window) override {
        if (window.GetID() == m_primary_window->GetID()) {
            Stop();
        }
    }

private:
    std::unique_ptr<BaseState> m_current_state;
    std::shared_ptr<sge::Renderer> m_renderer;
    std::shared_ptr<sge::GlfwWindow> m_primary_window;
    AppConfig m_config;
};

#endif