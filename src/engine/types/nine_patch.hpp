#ifndef _ENGINE_TYPES_NINE_PATCH_HPP_
#define _ENGINE_TYPES_NINE_PATCH_HPP_

#pragma once

#include <glm/gtc/quaternion.hpp>

#include "../math/rect.hpp"

#include "texture.hpp"
#include "anchor.hpp"

class NinePatch {
public:
    NinePatch() = default;

    NinePatch(Texture texture, glm::uvec4 margin) :
        m_texture(texture),
        m_margin(margin),
        m_size(texture.size()) {}

    inline NinePatch& set_texture(const Texture& texture) { m_texture = texture; return *this; }
    inline NinePatch& set_position(glm::vec2 position) { m_position = position; return *this; }
    inline NinePatch& set_rotation(glm::quat rotation) { m_rotation = rotation; return *this; }
    inline NinePatch& set_margin(glm::uvec4 margin) { m_margin = margin; return *this; }
    inline NinePatch& set_color(glm::vec4 color) { m_color = color; return *this; }
    inline NinePatch& set_scale(glm::vec2 scale) { m_scale = scale; return *this; }
    inline NinePatch& set_size(glm::vec2 size) { m_size = size; return *this; }
    inline NinePatch& set_flip_x(bool flip_x) { m_flip_x = flip_x; return *this; }
    inline NinePatch& set_flip_y(bool flip_y) { m_flip_y = flip_y; return *this; }
    inline NinePatch& set_anchor(Anchor anchor) { m_anchor = anchor; return *this; }

    [[nodiscard]] inline const Texture& texture() const { return m_texture; }
    [[nodiscard]] inline glm::vec2 position() const { return m_position; }
    [[nodiscard]] inline glm::quat rotation() const { return m_rotation; }
    [[nodiscard]] inline glm::uvec4 margin() const { return m_margin; }
    [[nodiscard]] inline glm::vec4 color() const { return m_color; }
    [[nodiscard]] inline glm::vec2 scale() const { return m_scale; }
    [[nodiscard]] inline bool flip_x() const { return m_flip_x; }
    [[nodiscard]] inline bool flip_y() const { return m_flip_y; }
    [[nodiscard]] inline Anchor anchor() const { return m_anchor; }

    [[nodiscard]] glm::vec2 size() const { return m_size * scale(); }

    [[nodiscard]] inline math::Rect calculate_aabb() const {
        return math::Rect::from_top_left(m_position - m_anchor.to_vec2() * size(), size());
    }

private:
    Texture m_texture;
    glm::quat m_rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::uvec4 m_margin = glm::uvec4(0); // Left Right Top Bottom
    glm::vec4 m_color = glm::vec4(1.0f);
    glm::vec2 m_position = glm::vec2(0.0f);
    glm::vec2 m_size = glm::vec2(1.0f);
    glm::vec2 m_scale = glm::vec2(1.0f);
    bool m_flip_x = false;
    bool m_flip_y = false;
    Anchor m_anchor = Anchor::Center;
};

#endif