#include "camera.h"

void Camera::update() {
    compute_projection_and_view_matrix();
    compute_transform_matrix();
}

void Camera::update_projection_area() {
    m_area = math::Rect::from_corners(
        -glm::vec2(m_viewport) / 2.0f * m_zoom,
        glm::vec2(m_viewport) / 2.0f * m_zoom
    );
}

void Camera::compute_projection_and_view_matrix() {
    const math::Rect projection_area = get_projection_area();

    const glm::mat4 ortho = glm::ortho(
        projection_area.min.x, projection_area.max.x,
        projection_area.min.y, projection_area.max.y,
        0.0f, 100.0f
    );

    const glm::mat4 projection_matrix = ortho;

    const glm::mat4 view_matrix = glm::lookAt(
        glm::vec3(m_position, -50.0),
        glm::vec3(m_position, 0.0),
        glm::vec3(0.0, -1.0, 0.0)
    );

    m_projection_matrix = projection_matrix;
    m_view_matrix = view_matrix;
}

void Camera::compute_transform_matrix() {
    m_transform_matrix = glm::translate(glm::mat4(1.0), glm::vec3(m_position, 0.));
}

inline glm::vec2 project_point(const glm::mat4& mat, const glm::vec2& point) {
    glm::vec4 res = mat[0] * point.x;
    res = mat[1] * point.y + res;
    res = mat[3] + res;
    return res;
}

glm::vec2 Camera::screen_to_world(const glm::vec2& screen_pos) const {
    const glm::vec2 ndc = screen_pos * 2.0f / glm::vec2(m_viewport) - glm::vec2(1.0);
    const glm::mat4 ndc_to_world = m_transform_matrix * glm::inverse(m_projection_matrix);
    const glm::vec2 world_pos = project_point(ndc_to_world, ndc);
    return world_pos;
}