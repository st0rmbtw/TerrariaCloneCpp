#ifndef TERRARIA_SPRITE_HPP
#define TERRARIA_SPRITE_HPP

#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include "../math/rect.hpp"
#include "../optional.hpp"
#include "anchor.hpp"
#include "texture.hpp"
#include "texture_atlas.hpp"

class BaseSprite {
public:
    BaseSprite() = default;
    ~BaseSprite() = default;

    explicit BaseSprite(glm::vec2 position) : m_position(position) {}

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
    inline BaseSprite& set_rotation(const glm::quat& rotation) { m_rotation = rotation; return *this; }
    inline BaseSprite& set_scale(const glm::vec2& scale) {
        m_scale = scale;
        calculate_aabb();
        return *this;
    }
    inline BaseSprite& set_color(const glm::vec4& color) { m_color = color; return *this; }
    inline BaseSprite& set_color(const glm::vec3& color) { m_color = glm::vec4(color, 1.0); return *this; }
    inline BaseSprite& set_outline_color(const glm::vec4& color) { m_outline_color = color; return *this; }
    inline BaseSprite& set_outline_color(const glm::vec3& color) { m_outline_color = glm::vec4(color, 1.0); return *this; }
    inline BaseSprite& set_outline_thickness(const float thickness) { m_outline_thickness = thickness; return *this; }
    inline BaseSprite& set_anchor(Anchor anchor) { m_anchor = anchor; return *this; }
    inline BaseSprite& set_flip_x(bool flip_x) { m_flip_x = flip_x; return *this; }
    inline BaseSprite& set_flip_y(bool flip_y) { m_flip_y = flip_y; return *this; }
    inline BaseSprite& set_order(int order) { m_order = order; return *this; }

    [[nodiscard]] inline const glm::vec2& position() const { return m_position; }
    [[nodiscard]] inline const glm::quat& rotation() const { return m_rotation; }
    [[nodiscard]] inline const glm::vec2& scale() const { return m_scale; }
    [[nodiscard]] inline const glm::vec4& color() const { return m_color; }
    [[nodiscard]] inline const glm::vec4& outline_color() const { return m_outline_color; }
    [[nodiscard]] inline float outline_thickness() const { return m_outline_thickness; }

    [[nodiscard]] inline Anchor anchor() const { return m_anchor; }
    [[nodiscard]] inline bool flip_x() const { return m_flip_x; }
    [[nodiscard]] inline bool flip_y() const { return m_flip_y; }

    [[nodiscard]] inline const math::Rect& aabb() const { return m_aabb; }
    [[nodiscard]] inline int order() const { return m_order; }

    [[nodiscard]] virtual glm::vec2 size() const = 0;

protected:
    void calculate_aabb() {
        m_aabb = math::Rect::from_top_left(m_position - anchor_to_vec2(m_anchor) * size(), size());
    }

private:
    math::Rect m_aabb;
    glm::quat m_rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec2 m_position = glm::vec2(0.0f);
    glm::vec2 m_scale = glm::vec2(1.0f);
    glm::vec4 m_color = glm::vec4(1.0f);
    glm::vec4 m_outline_color = glm::vec4(0.0f);
    float m_outline_thickness = 0.0f;
    int m_order = 0;
    Anchor m_anchor = Anchor::Center;
    bool m_flip_x = false;
    bool m_flip_y = false;
};

class Sprite : public BaseSprite {
public:
    Sprite() = default;

    Sprite& set_texture(Texture texture) { m_texture = texture; return *this; }
    BaseSprite& set_custom_size(tl::optional<glm::vec2> custom_size) {
        m_custom_size = custom_size;
        calculate_aabb();
        return *this;
    }

    [[nodiscard]] inline const tl::optional<Texture>& texture() const { return m_texture; }
    [[nodiscard]] inline const tl::optional<glm::vec2>& custom_size() const { return m_custom_size; }

    [[nodiscard]] glm::vec2 size() const override {
        auto size = glm::vec2(1.0f);

        if (m_texture.is_some()) size = m_texture->size();
        if (m_custom_size.is_some()) size = m_custom_size.value();

        return size * scale();
    }

private:
    tl::optional<glm::vec2> m_custom_size = tl::nullopt;
    tl::optional<Texture> m_texture = tl::nullopt;
};

class TextureAtlasSprite : public BaseSprite {
public:
    TextureAtlasSprite() :
        m_index(0) {}

    explicit TextureAtlasSprite(TextureAtlas texture_atlas) :
        m_texture_atlas(std::move(texture_atlas)),
        m_index(0) {}

    inline void set_index(size_t index) { m_index = index; }

    [[nodiscard]] inline size_t index() const { return m_index; }
    [[nodiscard]] inline const TextureAtlas& atlas() const { return m_texture_atlas; }

    [[nodiscard]] glm::vec2 size() const override {
        return m_texture_atlas.get_rect(m_index).size() * scale();
    }
private:
    TextureAtlas m_texture_atlas;
    size_t m_index;
};

#endif