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

#define SPRITE_SETTERS(_class, _override) \
    virtual inline _class& set_position(const glm::vec2& position) _override { m_position = position; return *this; } \
    virtual inline _class& set_rotation(const glm::quat& rotation) _override { m_rotation = rotation; return *this; } \
    virtual inline _class& set_scale(const glm::vec2& scale) _override { m_scale = scale; return *this; } \
    virtual inline _class& set_scale(const float scale) _override { m_scale = glm::vec2(scale); return *this; } \
    virtual inline _class& set_color(const glm::vec4& color) _override { m_color = color; return *this; } \
    virtual inline _class& set_color(const glm::vec3& color) _override { m_color = glm::vec4(color, 1.0); return *this; } \
    virtual inline _class& set_outline_color(const glm::vec4& color) _override { m_outline_color = color; return *this; } \
    virtual inline _class& set_outline_color(const glm::vec3& color) _override { m_outline_color = glm::vec4(color, 1.0); return *this; } \
    virtual inline _class& set_outline_thickness(const float thickness) _override { m_outline_thickness = thickness; return *this; } \
    virtual inline _class& set_anchor(Anchor anchor) _override { m_anchor = anchor; return *this; } \
    virtual inline _class& set_flip_x(bool flip_x) _override { m_flip_x = flip_x; return *this; } \
    virtual inline _class& set_flip_y(bool flip_y) _override { m_flip_y = flip_y; return *this; } \
    virtual inline _class& set_nonscalable(bool nonscalable) _override { m_nonscalable = nonscalable; return *this; } \


class BaseSprite {
protected:
    BaseSprite() = default;
    ~BaseSprite() = default;

    explicit BaseSprite(glm::vec2 position) : m_position(position) {}

    BaseSprite(glm::vec2 position, glm::vec2 scale) : 
        m_position(std::move(position)),
        m_scale(std::move(scale)) {}

    BaseSprite(glm::vec2 position, glm::vec2 scale, glm::vec4 color, Anchor anchor) : 
        m_position(std::move(position)),
        m_scale(std::move(scale)),
        m_color(std::move(color)),
        m_anchor(std::move(anchor)) {}

public:

    [[nodiscard]] inline const glm::vec2& position() const { return m_position; }
    [[nodiscard]] inline const glm::quat& rotation() const { return m_rotation; }
    [[nodiscard]] inline const glm::vec2& scale() const { return m_scale; }
    [[nodiscard]] inline const glm::vec4& color() const { return m_color; }
    [[nodiscard]] inline const glm::vec4& outline_color() const { return m_outline_color; }
    [[nodiscard]] inline float outline_thickness() const { return m_outline_thickness; }

    [[nodiscard]] inline Anchor anchor() const { return m_anchor; }
    [[nodiscard]] inline bool flip_x() const { return m_flip_x; }
    [[nodiscard]] inline bool flip_y() const { return m_flip_y; }
    [[nodiscard]] inline bool nonscalable() const { return m_nonscalable; }

    [[nodiscard]] virtual glm::vec2 size() const = 0;

    [[nodiscard]] inline math::Rect calculate_aabb() const {
        return math::Rect::from_top_left(m_position - m_anchor.to_vec2() * size(), size());
    }

    SPRITE_SETTERS(BaseSprite,)

protected:
    glm::quat m_rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec2 m_position = glm::vec2(0.0f);
    glm::vec2 m_scale = glm::vec2(1.0f);
    glm::vec4 m_color = glm::vec4(1.0f);
    glm::vec4 m_outline_color = glm::vec4(0.0f);
    float m_outline_thickness = 0.0f;
    Anchor m_anchor = Anchor::Center;
    bool m_flip_x = false;
    bool m_flip_y = false;
    /// Don't respect the zoom of the camera
    bool m_nonscalable = false;
};

class Sprite : public BaseSprite {
public:
    Sprite() = default;

    Sprite(glm::vec2 position) : BaseSprite(position) {}
    Sprite(glm::vec2 position, glm::vec2 scale) : BaseSprite(position, scale) {}

    inline Sprite& set_texture(Texture texture) { m_texture = texture; return *this; }
    inline BaseSprite& set_custom_size(tl::optional<glm::vec2> custom_size) { m_custom_size = custom_size; return *this; }

    [[nodiscard]] inline const tl::optional<Texture>& texture() const { return m_texture; }
    [[nodiscard]] inline const tl::optional<glm::vec2>& custom_size() const { return m_custom_size; }

    [[nodiscard]] glm::vec2 size() const override {
        auto size = glm::vec2(1.0f);

        if (m_texture.is_some()) size = m_texture->size;
        if (m_custom_size.is_some()) size = m_custom_size.value();

        return size * scale();
    }

    SPRITE_SETTERS(Sprite, override)

private:
    tl::optional<glm::vec2> m_custom_size = tl::nullopt;
    tl::optional<Texture> m_texture = tl::nullopt;
};

class TextureAtlasSprite : public BaseSprite {
public:
    TextureAtlasSprite() = default;

    explicit TextureAtlasSprite(TextureAtlas texture_atlas) :
        m_texture_atlas(std::move(texture_atlas)) {}

    TextureAtlasSprite(const TextureAtlas& texture_atlas, glm::vec2 position, glm::vec2 scale) : BaseSprite(position, scale)
    {
        m_texture_atlas = texture_atlas;
    }

    inline void set_index(size_t index) { m_index = index; }
    inline void set_index(uint16_t x, uint16_t y) { m_index = y * m_texture_atlas.columns() + x; }

    [[nodiscard]] inline size_t index() const { return m_index; }
    [[nodiscard]] inline const TextureAtlas& atlas() const { return m_texture_atlas; }

    [[nodiscard]] glm::vec2 size() const override {
        return glm::vec2(m_texture_atlas.size()) * scale();
    }

    SPRITE_SETTERS(TextureAtlasSprite, override)

private:
    TextureAtlas m_texture_atlas;
    size_t m_index = 0;
};

#endif