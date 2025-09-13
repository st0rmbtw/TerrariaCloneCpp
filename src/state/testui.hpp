#pragma once

#ifndef STATE_TESTUI_HPP_
#define STATE_TESTUI_HPP_

#include <SGE/renderer/camera.hpp>
#include <SGE/renderer/batch.hpp>

#include "base.hpp"

class TestUI : public BaseState {
public:
    TestUI();
    void Render() override;
    void Update() override;

    void OnWindowSizeChanged(glm::uvec2 size) override {
        m_camera.set_viewport(size);
        m_camera.update();
    }

    ~TestUI() override;

private:
    sge::Camera m_camera;
    sge::Batch m_batch;
};

#endif