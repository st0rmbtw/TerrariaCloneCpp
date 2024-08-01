#ifndef WORLD_WORLD_GEN_H
#define WORLD_WORLD_GEN_H

#pragma once

#include "world.hpp"

constexpr int DIRT_HILL_HEIGHT = 75;

WorldData world_generate(uint32_t width, uint32_t height, uint32_t seed);

#endif