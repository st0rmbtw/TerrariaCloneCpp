#ifndef WORLD_IO_TYPES_HPP_
#define WORLD_IO_TYPES_HPP_

#include <string>
#include <vector>

#include <SGE/math/rect.hpp>

#include "../../types/tile_pos.hpp"

#include "../types.hpp"

struct WorldHeader {
    std::string name;
    std::vector<bool> tile_frame_important;
    sge::IRect bounds;
    TilePos spawn_point;
    double surface_layer = 0.0;
    double rock_layer = 0.0;
    int32_t version = 0;
    int32_t id = 0;
    int32_t width = 0;
    int32_t height = 0;
    WorldEvil evil;
};

#endif