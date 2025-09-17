#pragma once

#ifndef STATE_MENU_HPP_
#define STATE_MENU_HPP_

#include <SGE/renderer/camera.hpp>
#include <SGE/renderer/batch.hpp>

#include "../renderer/background_renderer.hpp"

#include "common.hpp"
#include "base.hpp"

enum class MenuPosition : uint8_t {
    MainMenu = 0,
    SelectWorld,
    CreateWorld,
    Settings,
};

class MainMenuState : public BaseState {
public:
    MainMenuState();
    void Render() override;
    void Update() override;

    void OnWindowSizeChanged(glm::uvec2 size) override {
        m_camera.set_viewport(size);
        m_camera.update();
        setup_background();
    }

    BaseState* GetNextState() override;
    ~MainMenuState() override;

private:
    void setup_background();
    void draw_background(const BackgroundLayer& layer);
    void draw_ui();

    void draw_main_menu();
    void draw_select_world();

    void set_next_position(MenuPosition new_state);
    void set_previous_position();

private:
    sge::Camera m_camera;
    Cursor m_cursor;
    BackgroundRenderer m_background_renderer;
    sge::Batch m_batch;
    std::vector<BackgroundLayer> m_background_layers;

    std::list<MenuPosition> m_position_stack;

    bool m_exit = false;
    bool m_world_selected = false;
};

#endif