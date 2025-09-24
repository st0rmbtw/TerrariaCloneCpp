#pragma once

#ifndef STATE_MENU_HPP_
#define STATE_MENU_HPP_

#include <variant>
#include <SGE/renderer/camera.hpp>
#include <SGE/renderer/batch.hpp>
#include <SGE/math/quat.hpp>
#include <SGE/types/animation.hpp>
#include <SGE/time/stopwatch.hpp>
#include <SGE/time/timer.hpp>

#include "../../renderer/background_renderer.hpp"

#include "../common.hpp"
#include "../base.hpp"

#include "nav_manager.hpp"
#include "substate/create_world.hpp"
#include "substate/select_world.hpp"

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

    void update_logo();

private:
    sge::Camera m_camera;
    Cursor m_cursor;
    sge::Batch m_batch;

    std::variant<MenuSubstateCreateWorld, MenuSubstateSelectWorld> m_substate;

    BackgroundRenderer m_background_renderer;
    std::vector<BackgroundLayer> m_background_layers;

    NavManager m_nav_manager;

    glm::quat m_logo_rotation = Quat::from_rotation_z(glm::radians(-5.0f));
    sge::Animation m_logo_animation{ sge::Duration::SecondsFloat(10.0f), sge::RepeatStrategy::MirroredRepeat };

    float m_logo_scale = 0.9f;

    size_t m_prev_position = 0;
    bool m_exit = false;
};

#endif