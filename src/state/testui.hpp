#pragma once

#ifndef STATE_TESTUI_HPP_
#define STATE_TESTUI_HPP_

#include <SGE/renderer/camera.hpp>
#include <SGE/renderer/batch.hpp>

#include "base.hpp"

class TestUI : public BaseState {
public:
    TestUI(const std::shared_ptr<sge::Renderer>& renderer, uint8_t samples);
    void Update() override;
    void Render(const std::shared_ptr<sge::GlfwWindow>& window) override;

    void OnWindowSizeChanged(LLGL::Extent2D size) override {
        m_camera.set_viewport(size);
        m_camera.update();
    }

private:
    sge::Camera m_camera;
    sge::Batch m_batch;

    std::shared_ptr<sge::Renderer> m_renderer;
};

#endif