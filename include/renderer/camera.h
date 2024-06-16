#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "math/rect.hpp"
#include "constants.hpp"

class Camera {
public:
    Camera() :
        m_position(0.0f),
        m_viewport(0),
        m_area() {}

    Camera(const glm::uvec2& viewport) {
        m_viewport = viewport;
        update_projection_area();
        compute_projection_and_view_matrix();
    }

    void update();

    inline const glm::vec2& position() const { return m_position; }
    inline const glm::uvec2& viewport() const { return m_viewport; }
    inline const glm::mat4x4& get_projection_matrix() const { return m_projection_matrix; }
    inline const glm::mat4x4& get_view_matrix() const { return m_view_matrix; }
    inline const glm::mat4x4& get_transform_matrix() const { return m_transform_matrix; }
    inline const math::Rect& get_projection_area() const { return m_area; }
    inline const float zoom() const { return m_zoom; }

    void set_zoom(float zoom) {
        m_zoom = glm::clamp(zoom, Constants::CAMERA_MAX_ZOOM, Constants::CAMERA_MIN_ZOOM);
        update_projection_area();
    }

    void set_viewport(const glm::uvec2& viewport) {
        m_viewport = viewport;
        update_projection_area();
    }

    void set_position(const glm::vec2& position) {
        m_position = position;
    }

    glm::vec2 screen_to_world(const glm::vec2& screen_pos) const;
private:
    void compute_projection_and_view_matrix();
    void compute_transform_matrix();
    void update_projection_area();

private:
    glm::mat4x4 m_projection_matrix;
    glm::mat4x4 m_view_matrix;
    glm::mat4x4 m_transform_matrix;

    math::Rect m_area;

    glm::uvec2 m_viewport;
    glm::vec2 m_position;
    
    float m_zoom = 1.0f;
};