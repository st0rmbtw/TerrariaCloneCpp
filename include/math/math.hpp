#pragma once

#ifndef TERRARIA_MATH_MATH_HPP
#define TERRARIA_MATH_MATH_HPP

#include <glm/vec2.hpp>

float move_towards(float current, float target, float max_delta) noexcept;

inline int map_range(int in_min, int in_max, int out_min, int out_max, int x) noexcept {
    return out_min + (x - in_min) * (out_max - out_min) / (in_max - in_min);
}

inline float map_range(float in_min, float in_max, float out_min, float out_max, float x) noexcept {
    return out_min + (((x - in_min) / (in_max - in_min)) * (out_max - out_min));
}

glm::vec2 random_point_cone(glm::vec2 direction, float angle, float radius);
glm::vec2 random_point_cone(glm::vec2 direction, float angle);
glm::vec2 random_point_circle(float xradius, float yradius);

#endif