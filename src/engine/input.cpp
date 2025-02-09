#include <unordered_set>
#include "input.hpp"

static struct InputState {
    std::unordered_set<Key> keyboard_pressed;
    std::unordered_set<Key> keyboard_just_pressed;
    std::unordered_set<Key> keyboard_just_released;

    std::unordered_set<uint8_t> mouse_pressed;
    std::unordered_set<uint8_t> mouse_just_pressed;
    std::unordered_set<uint8_t> mouse_just_released;
    std::vector<float> mouse_scroll_events;
    glm::vec2 mouse_screen_position;
    bool mouse_over_ui = false;
} input_state;

void Input::Press(Key key) {
    if (input_state.keyboard_pressed.insert(key).second) {
        input_state.keyboard_just_pressed.insert(key);
    }
}

void Input::Release(Key key) {
    if (input_state.keyboard_pressed.erase(key) > 0) {
        input_state.keyboard_just_released.insert(key);
    }
}

bool Input::Pressed(Key key) {
    return input_state.keyboard_pressed.find(key) != input_state.keyboard_pressed.end();
}

bool Input::JustPressed(Key key) {
    return input_state.keyboard_just_pressed.find(key) != input_state.keyboard_just_pressed.end();
}

void Input::Clear() {
    input_state.keyboard_just_pressed.clear();
    input_state.keyboard_just_released.clear();

    input_state.mouse_just_pressed.clear();
    input_state.mouse_just_released.clear();
    input_state.mouse_scroll_events.clear();
    input_state.mouse_over_ui = false;
}


void Input::press(MouseButton button) {
    if (input_state.mouse_pressed.insert(static_cast<uint8_t>(button)).second) {
        input_state.mouse_just_pressed.insert(static_cast<uint8_t>(button));
    }
}

void Input::release(MouseButton button) {
    if (input_state.mouse_pressed.erase(static_cast<uint8_t>(button)) > 0) {
        input_state.mouse_just_released.insert(static_cast<uint8_t>(button));
    }
}

bool Input::Pressed(MouseButton button) {
    return input_state.mouse_pressed.find(static_cast<uint8_t>(button)) != input_state.mouse_pressed.end();
}

bool Input::JustPressed(MouseButton button) {
    return input_state.mouse_just_pressed.find(static_cast<uint8_t>(button)) != input_state.mouse_just_pressed.end();
}

void Input::PushMouseScrollEvent(float y) { input_state.mouse_scroll_events.push_back(y); }
void Input::SetMouseScreenPosition(const glm::vec2& position) { input_state.mouse_screen_position = position; }
void Input::SetMouseOverUi(bool mouse_over_ui) { input_state.mouse_over_ui = mouse_over_ui; }

const std::vector<float>& Input::ScrollEvents() { return input_state.mouse_scroll_events; }
const glm::vec2& Input::MouseScreenPosition() { return input_state.mouse_screen_position; }
bool Input::IsMouseOverUi() { return input_state.mouse_over_ui; }