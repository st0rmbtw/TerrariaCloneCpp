#pragma once

#ifndef STATE_MENU_SUBSTATE_CREATE_WORLD_HPP_
#define STATE_MENU_SUBSTATE_CREATE_WORLD_HPP_

#include "../../common.hpp"
#include "../nav_manager.hpp"

class MenuSubstateCreateWorld {
public:
    MenuSubstateCreateWorld() = default;

    void draw(NavManager& nav_manager);
    void update();

private:
    void set_random_seed();
    void set_random_name();

private:
    TextInputData m_name_input_data;
    TextInputData m_seed_input_data;

    sge::Timer m_backspace_timer = sge::Timer::from_seconds(0.5f, sge::TimerMode::Once);
    sge::Timer m_bar_timer = sge::Timer::from_seconds(0.5f, sge::TimerMode::Repeating);
    bool m_text_input_bar_visible = true;
};

#endif