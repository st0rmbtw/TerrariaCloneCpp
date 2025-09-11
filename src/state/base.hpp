#pragma once

#ifndef STATE_GAME_STATE_HPP_
#define STATE_GAME_STATE_HPP_

#include <glm/vec2.hpp>

// The base class for game states
class BaseState {
public:
    virtual void Render() = 0;
    virtual void Update() = 0;
    virtual void PostRender() {
    }
    virtual void PreUpdate() {
    }
    virtual void PostUpdate() {
    }
    virtual void FixedUpdate() {
    }
    virtual void OnWindowSizeChanged(glm::uvec2 /* size */) {
    }
    virtual BaseState* GetNextState() = 0;
    virtual ~BaseState() = default;
};

#endif