#ifndef TERRARIA_TYPES_BACKGROUND_LAYER
#define TERRARIA_TYPES_BACKGROUND_LAYER

#pragma once

#include <glm/glm.hpp>

#include "../types/anchor.hpp"
#include "../assets.hpp"

class BackgroundLayer {
public:
    BackgroundLayer(TextureAsset key, float scale) {
        const Texture& texture = Assets::GetTexture(key);

        m_texture = texture;
        m_scale = scale;
        m_size = glm::vec2(texture.size) * m_scale;
    }

    inline BackgroundLayer& set_width(float width) { m_size.x = width; return *this; }
    inline BackgroundLayer& set_height(float height) { m_size.y = height; return *this; }
    inline BackgroundLayer& set_position(const glm::vec2& position) { m_position = position; return *this; }
    inline BackgroundLayer& set_speed(float x, float y) { m_speed = glm::vec2(x, y); return *this; }
    inline BackgroundLayer& set_x(float x) { m_x = x; return *this; }
    inline BackgroundLayer& set_y(float y) { m_y = y;  return *this; }
    inline BackgroundLayer& set_anchor(const Anchor anchor) { m_anchor = anchor; return *this; }
    inline BackgroundLayer& set_nonscale(bool nonscale) { m_nonscale = nonscale; return *this; }
    inline BackgroundLayer& set_follow_camera(bool follow) { m_follow_camera = follow; return *this; }
    inline BackgroundLayer& set_fill_screen_height() { m_fill_screen_height = true; return *this; }
    inline BackgroundLayer& set_fill_screen_width() { m_fill_screen_width = true; return *this; }

    [[nodiscard]] inline const Texture& texture() const { return m_texture; }
    [[nodiscard]] inline const glm::vec2& position() const { return m_position; }
    [[nodiscard]] inline const glm::vec2& speed() const { return m_speed; }
    [[nodiscard]] inline const glm::vec2& size() const { return m_size; }
    [[nodiscard]] inline float scale() const { return m_scale; }
    [[nodiscard]] inline float x() const { return m_x; }
    [[nodiscard]] inline float y() const { return m_y; }
    [[nodiscard]] inline Anchor anchor() const { return m_anchor; }
    [[nodiscard]] inline uint32_t texture_id() const { return m_texture.id; }
    [[nodiscard]] inline bool nonscale() const { return m_nonscale; }
    [[nodiscard]] inline bool follow_camera() const { return m_follow_camera; }
    [[nodiscard]] inline bool fill_screen_height() const { return m_fill_screen_height; }
    [[nodiscard]] inline bool fill_screen_width() const { return m_fill_screen_width; }

private:
    Texture m_texture;
    glm::vec2 m_position = glm::vec2(0.0f);
    glm::vec2 m_speed = glm::vec2(0.0f);
    glm::vec2 m_size = glm::vec2(0.0f);
    float m_scale = 0.0f;
    float m_x;
    float m_y;
    Anchor m_anchor;
    bool m_nonscale = true;
    bool m_follow_camera = true;
    bool m_fill_screen_height = false;
    bool m_fill_screen_width = false;
};

#endif