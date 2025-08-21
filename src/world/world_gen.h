#pragma once

#ifndef WORLD_WORLD_GEN_H_
#define WORLD_WORLD_GEN_H_

#include "world_data.hpp"

void world_generate(WorldData& world, uint32_t width, uint32_t height, uint32_t seed);

#endif