#pragma once

#ifndef TERRARIA_SPRITE_HPP
#define TERRARIA_SPRITE_HPP

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include "types/anchor.hpp"
// #include "texture.h"
#include "optional.hpp"
#include "math/rect.hpp"

class Sprite {
public:
    Sprite() :
        m_position(0.0),
        m_scale(1.0),
        m_rotation(1.0, 0.0, 0.0, 0.0),
        m_color(1.0),
        m_outline_color(glm::vec4(0.0)),
        m_outline_thickness(0.0),
        m_custom_size(),
        m_anchor(Anchor::Center),
        m_flip_x(false),
        m_flip_y(false) {}

    Sprite(glm::vec2 position) : 
        m_position(position),
        m_scale(1.0),
        m_rotation(1.0, 0.0, 0.0, 0.0),
        m_color(1.0),
        m_outline_color(glm::vec4(0.0)),
        m_outline_thickness(0.0),
        m_custom_size(),
        m_anchor(Anchor::Center),
        m_flip_x(false),
        m_flip_y(false) {}

    Sprite(glm::vec2 position, glm::vec2 scale, glm::vec4 color, Anchor anchor) : 
        m_position(position),
        m_scale(scale),
        m_rotation(1.0, 0.0, 0.0, 0.0),
        m_color(color),
        m_outline_color(glm::vec4(0.0)),
        m_outline_thickness(0.0),
        m_custom_size(),
        m_anchor(anchor),
        m_flip_x(false),
        m_flip_y(false) {}

    Sprite& set_position(const glm::vec2& position) {
        m_position = position;
        calculate_aabb();
        return *this;
    }
    Sprite& set_rotation(const glm::quat& rotation) { m_rotation = rotation; return *this; }
    Sprite& set_scale(const glm::vec2& scale) { m_scale = scale; return *this; }
    Sprite& set_color(const glm::vec4& color) { m_color = color; return *this; }
    Sprite& set_color(const glm::vec3& color) { m_color = glm::vec4(color, 1.0); return *this; }
    Sprite& set_outline_color(const glm::vec4& color) { m_outline_color = color; return *this; }
    Sprite& set_outline_color(const glm::vec3& color) { m_outline_color = glm::vec4(color, 1.0); return *this; }
    Sprite& set_outline_thickness(const float thickness) { m_outline_thickness = thickness; return *this; }
    // Sprite& set_texture(const Texture* texture) { m_texture = texture; return *this; }
    Sprite& set_anchor(Anchor anchor) { m_anchor = anchor; return *this; }
    Sprite& set_flip_x(bool flip_x) { m_flip_x = flip_x; return *this; }
    Sprite& set_flip_y(bool flip_y) { m_flip_y = flip_y; return *this; }
    Sprite& set_custom_size(tl::optional<glm::vec2> custom_size) {
        m_custom_size = custom_size;
        calculate_aabb();
        return *this;
    }

    const glm::vec2& position(void) const { return m_position; }
    const glm::quat& rotation(void) const { return m_rotation; }
    const glm::vec2& scale(void) const { return m_scale; }
    const glm::vec4& color(void) const { return m_color; }
    const glm::vec4& outline_color(void) const { return m_outline_color; }
    const float outline_thickness(void) const { return m_outline_thickness; }
    const tl::optional<glm::vec2>& custom_size(void) const { return m_custom_size; }

    Anchor anchor(void) const { return m_anchor; }
    bool flip_x(void) const { return m_flip_x; }
    bool flip_y(void) const { return m_flip_y; }
    
    // const Texture* texture(void) const { return m_texture; }

    const math::Rect& aabb(void) const { return m_aabb; }

private:
    void calculate_aabb(void) {
        glm::vec2 size = glm::vec2(0.0);

        if (m_custom_size.is_some()) {
            size = m_custom_size.value();
        }

        size = size * m_scale;

        m_aabb = math::Rect::from_top_left(m_position - anchor_to_vec2(m_anchor) * size, size);
    }

private:
    glm::vec2 m_position;
    glm::vec2 m_scale;
    glm::quat m_rotation;
    glm::vec4 m_color;
    glm::vec4 m_outline_color;
    float m_outline_thickness;
    tl::optional<glm::vec2> m_custom_size;
    math::Rect m_aabb;
    Anchor m_anchor;
    bool m_flip_x;
    bool m_flip_y;
    // const Texture* m_texture;
};

#endif