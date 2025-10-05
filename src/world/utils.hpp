#pragma once

#ifndef WORLD_UTILS_HPP_
#define WORLD_UTILS_HPP_

#include <glm/glm.hpp>

#include <SGE/renderer/camera.hpp>
#include <SGE/math/rect.hpp>

namespace utils {
    inline sge::Rect get_camera_fov(const sge::Camera& camera) noexcept {
        const glm::vec2& camera_pos = camera.position();
        const sge::Rect& projection_area = camera.get_projection_area();
        return {camera_pos + projection_area.min, camera_pos + projection_area.max};
    }
};

#endif