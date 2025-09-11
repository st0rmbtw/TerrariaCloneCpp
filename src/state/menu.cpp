#include "menu.hpp"

#include "../app.hpp"

MenuState::MenuState() :
    m_camera{ sge::CameraOrigin::Center, sge::CoordinateSystem {
        .up = sge::CoordinateDirectionY::Negative,
        .forward = sge::CoordinateDirectionZ::Negative,
    }}
{
    m_camera.set_viewport(App::GetWindowResolution());
    m_camera.set_zoom(1.0f);
}

MenuState::~MenuState() {
}

void MenuState::Render() {
}

void MenuState::Update() {
}

BaseState* MenuState::GetNextState() {
    return nullptr;
}