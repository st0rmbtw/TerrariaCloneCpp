#ifndef TERRARIA_INPUT_HPP
#define TERRARIA_INPUT_HPP

#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <stdint.h>
#include <vector>

enum class MouseButton : uint8_t {
    Left = GLFW_MOUSE_BUTTON_LEFT,
    Middle = GLFW_MOUSE_BUTTON_MIDDLE,
    Right = GLFW_MOUSE_BUTTON_RIGHT,
};

enum class Key : uint16_t {
    Q = GLFW_KEY_Q,
    W = GLFW_KEY_W,
    E = GLFW_KEY_E,
    R = GLFW_KEY_R,
    T = GLFW_KEY_T,
    Y = GLFW_KEY_Y,
    U = GLFW_KEY_U,
    I = GLFW_KEY_I,
    O = GLFW_KEY_O,
    P = GLFW_KEY_P,
    A = GLFW_KEY_A,
    S = GLFW_KEY_S,
    D = GLFW_KEY_D,
    F = GLFW_KEY_F,
    G = GLFW_KEY_G,
    H = GLFW_KEY_H,
    J = GLFW_KEY_J,
    K = GLFW_KEY_K,
    L = GLFW_KEY_L,
    Z = GLFW_KEY_Z,
    X = GLFW_KEY_X,
    C = GLFW_KEY_C,
    V = GLFW_KEY_V,
    B = GLFW_KEY_B,
    N = GLFW_KEY_N,
    M = GLFW_KEY_M,

    Digit1 = GLFW_KEY_1,
    Digit2 = GLFW_KEY_2,
    Digit3 = GLFW_KEY_3,
    Digit4 = GLFW_KEY_4,
    Digit5 = GLFW_KEY_5,
    Digit6 = GLFW_KEY_6,
    Digit7 = GLFW_KEY_7,
    Digit8 = GLFW_KEY_8,
    Digit9 = GLFW_KEY_9,
    Digit0 = GLFW_KEY_0,
    
    LeftShift = GLFW_KEY_LEFT_SHIFT,
    LeftAlt = GLFW_KEY_LEFT_ALT,
    RightShift = GLFW_KEY_RIGHT_SHIFT,
    RightAlt = GLFW_KEY_RIGHT_ALT,

    Minus = GLFW_KEY_MINUS,
    Equals = GLFW_KEY_EQUAL,
    Space = GLFW_KEY_SPACE,

    Escape = GLFW_KEY_ESCAPE,

    F10 = GLFW_KEY_F10
};

namespace Input {
    void Press(Key key);
    void Release(Key key);

    bool Pressed(Key key);
    bool JustPressed(Key key);


    void press(MouseButton button);
    void release(MouseButton button);

    bool Pressed(MouseButton button);
    bool JustPressed(MouseButton button);

    void PushMouseScrollEvent(float y);
    void SetMouseScreenPosition(const glm::vec2& position);
    void SetMouseOverUi(bool mouse_over_ui);

    const std::vector<float>& ScrollEvents();
    const glm::vec2& MouseScreenPosition();
    bool IsMouseOverUi();

    void Clear();
};

#endif