#pragma once

#ifndef STATE_BASE_HPP_
#define STATE_BASE_HPP_

#include <memory>
#include <glm/vec2.hpp>

#include <SGE/renderer/glfw_window.hpp>

// The base class for game states
class BaseState {
public:
    virtual void Update() = 0;
    virtual void Render(const std::shared_ptr<sge::GlfwWindow>& window) = 0;
    virtual void PostRender() {
    }
    virtual void PreUpdate() {
    }
    virtual void PostUpdate() {
    }
    virtual void FixedUpdate() {
    }
    virtual void OnWindowSizeChanged(LLGL::Extent2D /* size */) {
    }
    virtual void OnFramebufferSizeChanged(LLGL::Extent2D /* size */) {
    }
    virtual BaseState* GetNextState() {
        return this;
    }
    virtual ~BaseState() = default;
};

#endif