#pragma once

#ifndef STATE_MENU_SUBSTATE_CREATE_WORLD_HPP_
#define STATE_MENU_SUBSTATE_CREATE_WORLD_HPP_

#include "../nav_manager.hpp"

#include "../../../ui/text_input_data.hpp"

class MenuSubstateCreateWorld {
public:
    static constexpr uint32_t SEED_LENGTH = 40;

    MenuSubstateCreateWorld();

    void draw(NavManager& nav_manager);

private:
    void set_random_seed();
    void set_random_name();

    bool validate_input();

private:
    TextInputData m_name_input_data{ 255 };
    TextInputData m_seed_input_data{ SEED_LENGTH };

    TextInputData m_world_width_input_data{ 10 };
    TextInputData m_world_height_input_data{ 10 };

    sge::Timer m_backspace_timer = sge::Timer::from_seconds(0.5f, sge::TimerMode::Once);
};

#endif