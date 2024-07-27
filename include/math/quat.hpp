#pragma once

#ifndef TERRARIA_MATH_QUAT_HPP
#define TERRARIA_MATH_QUAT_HPP

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Quat {
    static inline glm::quat from_rotation_z(float angle) {
        const float s = glm::sin(angle * 0.5f);
        const float c = glm::cos(angle * 0.5f);
        return glm::quat(c, 0.0f, 0.0f, -s);
    }
}

#endif