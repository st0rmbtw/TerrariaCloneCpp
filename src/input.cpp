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

void KeyboardInput::press(Key key) {
    if (input_state.keyboard_pressed.insert(key).second) {
        input_state.keyboard_just_pressed.insert(key);
    }
}

void KeyboardInput::release(Key key) {
    if (input_state.keyboard_pressed.erase(key) > 0) {
        input_state.keyboard_just_released.insert(key);
    }
}

bool KeyboardInput::Pressed(Key key) {
    return input_state.keyboard_pressed.find(key) != input_state.keyboard_pressed.end();
}

bool KeyboardInput::JustPressed(Key key) {
    return input_state.keyboard_just_pressed.find(key) != input_state.keyboard_just_pressed.end();
}

void KeyboardInput::Clear() {
    input_state.keyboard_just_pressed.clear();
    input_state.keyboard_just_released.clear();
}


void MouseInput::press(MouseButton button) {
    if (input_state.mouse_pressed.insert(static_cast<uint8_t>(button)).second) {
        input_state.mouse_just_pressed.insert(static_cast<uint8_t>(button));
    }
}

void MouseInput::release(MouseButton button) {
    if (input_state.mouse_pressed.erase(static_cast<uint8_t>(button)) > 0) {
        input_state.mouse_just_released.insert(static_cast<uint8_t>(button));
    }
}

bool MouseInput::Pressed(MouseButton button) {
    return input_state.mouse_pressed.find(static_cast<uint8_t>(button)) != input_state.mouse_pressed.end();
}

bool MouseInput::JustPressed(MouseButton button) {
    return input_state.mouse_just_pressed.find(static_cast<uint8_t>(button)) != input_state.mouse_just_pressed.end();
}

void MouseInput::Clear() {
    input_state.mouse_just_pressed.clear();
    input_state.mouse_just_released.clear();
    input_state.mouse_scroll_events.clear();
    input_state.mouse_over_ui = false;
}

const std::vector<float>& MouseInput::scroll_events(void) { return input_state.mouse_scroll_events; }

void MouseInput::push_scroll_event(float y) { input_state.mouse_scroll_events.push_back(y); }
void MouseInput::set_screen_position(const glm::vec2& position) { input_state.mouse_screen_position = position; }
void MouseInput::set_mouse_over_ui(bool mouse_over_ui) { input_state.mouse_over_ui = mouse_over_ui; }

const glm::vec2& MouseInput::ScreenPosition(void) { return input_state.mouse_screen_position; }
bool MouseInput::IsMouseOverUi(void) { return input_state.mouse_over_ui; }