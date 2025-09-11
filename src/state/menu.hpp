#pragma once

#ifndef STATE_MENU_STATE_HPP_
#define STATE_MENU_STATE_HPP_

#include <SGE/renderer/camera.hpp>

#include "base.hpp"

class MenuState : public BaseState {
public:
    MenuState();
    void Render() override;
    void Update() override;

    void OnWindowSizeChanged(glm::uvec2 size) override {
        m_camera.set_viewport(size);
        m_camera.update();
    }

    BaseState* GetNextState() override;
    ~MenuState() override;

private:
    sge::Camera m_camera;
};

#endif