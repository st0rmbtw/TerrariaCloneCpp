#ifndef WORLD_UTILS_HPP
#define WORLD_UTILS_HPP

#include <glm/glm.hpp>

#include "../engine/renderer/camera.hpp"
#include "../engine/math/rect.hpp"

#include "../types/tile_pos.hpp"
#include "../constants.hpp"

namespace utils {
    inline glm::uvec2 get_chunk_pos(TilePos tile_pos) noexcept {
        return glm::uvec2(tile_pos.x, tile_pos.y) / static_cast<uint32_t>(Constants::RENDER_CHUNK_SIZE_U);
    }

    inline math::Rect get_camera_fov(const Camera& camera) {
        const glm::vec2& camera_pos = camera.position();
        const math::Rect& projection_area = camera.get_projection_area();
        return {camera_pos + projection_area.min, camera_pos + projection_area.max};
    }
};

#endif