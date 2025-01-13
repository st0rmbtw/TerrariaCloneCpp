#ifndef MATH_MATH_HPP
#define MATH_MATH_HPP

#pragma once

#include <glm/vec2.hpp>

[[nodiscard]]
float move_towards(float current, float target, float max_delta) noexcept;

[[nodiscard]]
inline int map_range(int in_min, int in_max, int out_min, int out_max, int x) noexcept {
    return out_min + (x - in_min) * (out_max - out_min) / (in_max - in_min);
}

[[nodiscard]]
inline float map_range(float in_min, float in_max, float out_min, float out_max, float x) noexcept {
    return out_min + (((x - in_min) / (in_max - in_min)) * (out_max - out_min));
}

[[nodiscard]]
glm::vec2 random_point_cone(glm::vec2 direction, float angle, float radius);

[[nodiscard]]
glm::vec2 random_point_cone(glm::vec2 direction, float angle);

[[nodiscard]]
glm::vec2 random_point_circle(float xradius, float yradius);

#endif