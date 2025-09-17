#pragma once

#ifndef STATE_COMMON_HPP_
#define STATE_COMMON_HPP_

#include <SGE/types/color.hpp>
#include <SGE/types/sprite.hpp>
#include <SGE/renderer/batch.hpp>
#include <glm/vec2.hpp>

#include "../assets.hpp"

enum class AnimationDirection : uint8_t {
    Forward = 0,
    Backward = 1
};

class Cursor {
public:
    Cursor() {
        m_background
            .set_texture(Assets::GetTexture(TextureAsset::UiCursorBackground))
            .set_anchor(sge::Anchor::TopLeft)
            .set_outline_thickness(0.03f);

        m_foreground
            .set_texture(Assets::GetTexture(TextureAsset::UiCursorForeground))
            .set_anchor(sge::Anchor::TopLeft);
    }

    void Update(glm::vec2 mouse_position);

    inline void SetBackgroundColor(sge::LinearRgba color) {
        m_background_color = color;
        m_background.set_color(color);
        m_background.set_outline_color(color);
    }

    inline void SetForegroundColor(sge::LinearRgba color) {
        m_foreground_color = color;
        m_foreground.set_color(color);
    }

    [[nodiscard]]
    inline const sge::Sprite& Background() const noexcept {
        return m_background;
    }

    [[nodiscard]]
    inline const sge::Sprite& Foreground() const noexcept {
        return m_foreground;
    }
    
    [[nodiscard]]
    inline float Scale() const noexcept {
        return m_scale;
    }

private:
    sge::Sprite m_foreground;
    sge::Sprite m_background;

    sge::LinearRgba m_foreground_color;
    sge::LinearRgba m_background_color;
    
    float m_anim_progress = 0.0f;
    float m_scale = 1.0f;
    
    AnimationDirection m_anim_dir;
};

#endif