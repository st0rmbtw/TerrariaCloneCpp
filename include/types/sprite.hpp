#pragma once

#ifndef TERRARIA_SPRITE_HPP
#define TERRARIA_SPRITE_HPP

#include <LLGL/LLGL.h>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include "types/anchor.hpp"
#include "types/texture.hpp"
#include "types/texture_atlas.hpp"
#include "optional.hpp"
#include "math/rect.hpp"
#include "optional.hpp"

class BaseSprite {
public:
    BaseSprite() {}

    BaseSprite(glm::vec2 position) : m_position(position) {}

    BaseSprite(glm::vec2 position, glm::vec2 scale, glm::vec4 color, Anchor anchor) : 
        m_position(position),
        m_scale(scale),
        m_color(color),
        m_anchor(anchor) {}

    BaseSprite& set_position(const glm::vec2& position) {
        m_position = position;
        calculate_aabb();
        return *this;
    }
    BaseSprite& set_rotation(const glm::quat& rotation) { m_rotation = rotation; return *this; }
    BaseSprite& set_scale(const glm::vec2& scale) { m_scale = scale; return *this; }
    BaseSprite& set_color(const glm::vec4& color) { m_color = color; return *this; }
    BaseSprite& set_color(const glm::vec3& color) { m_color = glm::vec4(color, 1.0); return *this; }
    BaseSprite& set_outline_color(const glm::vec4& color) { m_outline_color = color; return *this; }
    BaseSprite& set_outline_color(const glm::vec3& color) { m_outline_color = glm::vec4(color, 1.0); return *this; }
    BaseSprite& set_outline_thickness(const float thickness) { m_outline_thickness = thickness; return *this; }
    BaseSprite& set_anchor(Anchor anchor) { m_anchor = anchor; return *this; }
    BaseSprite& set_flip_x(bool flip_x) { m_flip_x = flip_x; return *this; }
    BaseSprite& set_flip_y(bool flip_y) { m_flip_y = flip_y; return *this; }

    const glm::vec2& position(void) const { return m_position; }
    const glm::quat& rotation(void) const { return m_rotation; }
    const glm::vec2& scale(void) const { return m_scale; }
    const glm::vec4& color(void) const { return m_color; }
    const glm::vec4& outline_color(void) const { return m_outline_color; }
    const float outline_thickness(void) const { return m_outline_thickness; }

    Anchor anchor(void) const { return m_anchor; }
    bool flip_x(void) const { return m_flip_x; }
    bool flip_y(void) const { return m_flip_y; }

    const math::Rect& aabb(void) const { return m_aabb; }

    virtual glm::vec2 size(void) const = 0;

protected:
    void calculate_aabb(void) {
        m_aabb = math::Rect::from_top_left(m_position - anchor_to_vec2(m_anchor) * size(), size());
    }

protected:
    glm::vec2 m_position = glm::vec2(0.0f);
    glm::vec2 m_scale = glm::vec2(1.0f);
    glm::quat m_rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec4 m_color = glm::vec4(1.0f);
    glm::vec4 m_outline_color = glm::vec4(0.0f);
    float m_outline_thickness = 0.0f;
    math::Rect m_aabb = math::Rect();
    Anchor m_anchor = Anchor::Center;
    bool m_flip_x = false;
    bool m_flip_y = false;
};

class Sprite : public BaseSprite {
public:
    Sprite() {}

    Sprite& set_texture(Texture texture) { m_texture = texture; return *this; }
    BaseSprite& set_custom_size(tl::optional<glm::vec2> custom_size) {
        m_custom_size = custom_size;
        calculate_aabb();
        return *this;
    }

    const tl::optional<Texture>& texture(void) const { return m_texture; }
    const tl::optional<glm::vec2>& custom_size(void) const { return m_custom_size; }

    virtual glm::vec2 size(void) const override {
        glm::vec2 size = glm::vec2(1.0f);

        if (m_texture.is_some()) size = m_texture->size();
        if (m_custom_size.is_some()) size = m_custom_size.value();

        return size * m_scale;
    }

private:
    tl::optional<glm::vec2> m_custom_size = tl::nullopt;
    tl::optional<Texture> m_texture = tl::nullopt;
};

class TextureAtlasSprite : public BaseSprite {
public:
    TextureAtlasSprite(const TextureAtlas& texture_atlas) :
        m_texture_atlas(texture_atlas),
        m_index(0) {}

    void set_index(size_t index) {m_index = index; }

    size_t index(void) const { return m_index; }
    const TextureAtlas& atlas(void) const { return m_texture_atlas; }

    virtual glm::vec2 size(void) const override {
        return m_texture_atlas.rects()[m_index].size() * m_scale;
    }
private:
    TextureAtlas m_texture_atlas;
    size_t m_index;
};

#endif