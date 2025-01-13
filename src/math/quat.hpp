#pragma once

#ifndef MATH_QUAT_HPP
#define MATH_QUAT_HPP

#include <glm/gtx/quaternion.hpp>

namespace Quat {
    [[nodiscard]]
    inline glm::quat from_rotation_z(float angle) {
        const float s = glm::sin(angle * 0.5f);
        const float c = glm::cos(angle * 0.5f);
        return {c, 0.0f, 0.0f, -s};
    }
}

#endif