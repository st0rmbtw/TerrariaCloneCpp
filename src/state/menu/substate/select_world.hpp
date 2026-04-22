#pragma once

#ifndef STATE_MENU_SUBSTATE_HPP_
#define STATE_MENU_SUBSTATE_HPP_

#include <vector>
#include <filesystem>

#include "../nav_manager.hpp"

#include "../../../assets.hpp"

struct WorldInfo {
    std::filesystem::path path;
    std::string name;
    TextureAsset icon;
};

class MenuSubstateSelectWorld {
public:
    MenuSubstateSelectWorld();

    void draw(NavManager& nav_manager);

private:
    void get_worlds();
    void read_world(const std::filesystem::path& path);
private:

    std::vector<WorldInfo> m_worlds;
};

#endif