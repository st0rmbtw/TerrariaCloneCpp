#pragma once

#ifndef STATE_MENU_SUBSTATE_HPP_
#define STATE_MENU_SUBSTATE_HPP_

#include "../nav_manager.hpp"

class MenuSubstateSelectWorld {
public:
    MenuSubstateSelectWorld() = default;

    void draw(NavManager& nav_manager);
    void update() {}
};

#endif