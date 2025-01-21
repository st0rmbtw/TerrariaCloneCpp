#ifndef TYPES_BACKGROUND_LAYER
#define TYPES_BACKGROUND_LAYER

#pragma once

#include <glm/glm.hpp>

#include "../types/anchor.hpp"
#include "../assets.hpp"

class BackgroundLayer {
public:
    BackgroundLayer(BackgroundAsset key, float scale) {
        const uint16_t id = static_cast<uint16_t>(key);

        const TextureAtlas& atlas = Assets::GetTextureAtlas(TextureAsset::Backgrounds);
        const glm::vec2 rect_size = atlas.rects()[id].size();
        const glm::vec2 size = id == 0 ? glm::vec2(rect_size.y, rect_size.x) : rect_size;

        m_id = id;
        m_scale = scale;
        m_size = size * scale;
        m_texture_size = rect_size;
    }

    inline BackgroundLayer& set_width(float width) { m_size.x = width; return *this; }
    inline BackgroundLayer& set_height(float height) { m_size.y = height; return *this; }
    inline BackgroundLayer& set_position(const glm::vec2& position) { m_position = position; return *this; }
    inline BackgroundLayer& set_speed(float x, float y) { m_speed = glm::vec2(x, y); return *this; }
    inline BackgroundLayer& set_x(float x) { m_x = x; return *this; }
    inline BackgroundLayer& set_y(float y) { m_y = y; m_position.y = y; return *this; }
    inline BackgroundLayer& set_anchor(const Anchor anchor) { m_anchor = anchor; return *this; }
    inline BackgroundLayer& set_nonscale(bool nonscale) { m_nonscale = nonscale; return *this; }
    inline BackgroundLayer& set_follow_camera(bool follow) { m_follow_camera = follow; return *this; }
    inline BackgroundLayer& set_fill_screen_height(bool fill) { m_fill_screen_height = fill; return *this; }
    inline BackgroundLayer& set_fill_screen_width(bool fill) { m_fill_screen_width = fill; return *this; }
    inline BackgroundLayer& set_world_background() { m_world = true; return *this; }
    inline BackgroundLayer& set_surface_layer(bool surface_layer) { m_surface_layer = surface_layer; return *this; }

    [[nodiscard]] inline const glm::vec2& position() const { return m_position; }
    [[nodiscard]] inline const glm::vec2& speed() const { return m_speed; }
    [[nodiscard]] inline const glm::vec2& size() const { return m_size; }
    [[nodiscard]] inline glm::vec2 texture_size() const {
        return m_id == 0 ? glm::vec2(m_texture_size.y, m_texture_size.x) : m_texture_size;
    }
    [[nodiscard]] inline float scale() const { return m_scale; }
    [[nodiscard]] inline float x() const { return m_x; }
    [[nodiscard]] inline float y() const { return m_y; }
    [[nodiscard]] inline Anchor anchor() const { return m_anchor; }
    [[nodiscard]] inline bool nonscale() const { return m_nonscale; }
    [[nodiscard]] inline bool follow_camera() const { return m_follow_camera; }
    [[nodiscard]] inline bool fill_screen_height() const { return m_fill_screen_height; }
    [[nodiscard]] inline bool fill_screen_width() const { return m_fill_screen_width; }
    [[nodiscard]] inline bool is_world() const { return m_world; }
    [[nodiscard]] inline bool is_surface_layer() const { return m_surface_layer; }
    [[nodiscard]] inline uint16_t id() const { return m_id; }

private:
    glm::vec2 m_position = glm::vec2(0.0f);
    glm::vec2 m_speed = glm::vec2(0.0f);
    glm::vec2 m_size = glm::vec2(0.0f);
    glm::vec2 m_texture_size = glm::vec2(0.0f);
    uint16_t m_id = 0;
    float m_scale = 0.0f;
    float m_x;
    float m_y;
    Anchor m_anchor;
    bool m_nonscale = true;
    bool m_follow_camera = true;
    bool m_fill_screen_height = false;
    bool m_fill_screen_width = false;
    bool m_world = false;
    bool m_surface_layer = false;
};

#endif