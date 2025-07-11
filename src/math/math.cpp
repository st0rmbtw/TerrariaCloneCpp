#include "math.hpp"

#include <glm/glm.hpp>

#include <SGE/assert.hpp>
#include <SGE/math/consts.hpp>

#include "../utils.hpp"

float move_towards(float current, float target, float max_delta) noexcept {
    if (glm::abs(target - current) <= max_delta) {
        return target;
    }

    return current + glm::sign(target - current) * max_delta;
}

glm::vec2 random_point_cone(glm::vec2 direction, float angle, float radius) {
    SGE_ASSERT(-1.0f <= direction.x && direction.x <= 1.0f);
    SGE_ASSERT(-1.0f <= direction.y && direction.y <= 1.0f);
    SGE_ASSERT(0.0f <= angle && angle <= 180.0f);

    const float rad = glm::radians(angle);
    const float a = glm::atan(direction.y, direction.x);
    const float start_angle = -(rad / 2.0f) + a;
    const float end_angle = (rad / 2.0f) + a;
    const float theta = rand_range(start_angle, end_angle);
    const float x = glm::cos(theta);
    const float y = glm::sin(theta);

    return glm::vec2(x, y) * rand_range(0.0f, radius);
}

glm::vec2 random_point_cone(glm::vec2 direction, float angle) {
    SGE_ASSERT(-1.0f <= direction.x && direction.x <= 1.0f);
    SGE_ASSERT(-1.0f <= direction.y && direction.y <= 1.0f);
    SGE_ASSERT(0.0f <= angle && angle <= 180.0f);

    const float rad = glm::radians(angle);
    const float a = glm::atan(direction.y, direction.x);
    const float start_angle = -(rad / 2.0f) + a;
    const float end_angle = (rad / 2.0f) + a;
    const float theta = rand_range(start_angle, end_angle);
    const float x = glm::cos(theta);
    const float y = glm::sin(theta);

    return glm::vec2(x, y);
}

glm::vec2 random_point_circle(float xradius, float yradius) {
    SGE_ASSERT(0.0f <= xradius && xradius <= 1.0f);
    SGE_ASSERT(0.0f <= yradius && yradius <= 1.0f);

    const glm::vec2 radius = glm::vec2(xradius, yradius) * glm::sqrt(rand_range(0.0f, 1.0f));
    const float theta = rand_range(0.0f, 1.0f) * 2.0f * sge::consts::PI;
    const float x = radius.x * glm::cos(theta);
    const float y = radius.y * glm::sin(theta);

    return glm::vec2(x, y);
}