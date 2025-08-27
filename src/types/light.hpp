#pragma once

#ifndef TYPES_LIGHT_HPP_
#define TYPES_LIGHT_HPP_

#include <glm/vec3.hpp>

#include "tile_pos.hpp"

struct Light {
    glm::vec3 color; alignas(16)
    TilePos pos;
    glm::uvec2 size;
};

#endif