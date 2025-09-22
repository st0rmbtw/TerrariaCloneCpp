#pragma once

#ifndef STATE_COMMON_HPP_
#define STATE_COMMON_HPP_

#include <SGE/types/color.hpp>
#include <SGE/types/sprite.hpp>
#include <SGE/renderer/batch.hpp>
#include <SGE/types/animation.hpp>
#include <SGE/time/timer.hpp>

#include <glm/vec2.hpp>

#include "../assets.hpp"

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

    sge::Animation m_animation{ sge::Duration::SecondsFloat(1.0f), sge::RepeatStrategy::MirroredRepeat };
    
    float m_scale = 1.0f;
};

class TextInputData {
public:
    TextInputData() = default;
    TextInputData(std::function<bool(uint32_t)> filter) noexcept :
        m_filter_function(std::move(filter)) {};

    void update() noexcept;

    void clear() noexcept {
        m_data.clear();
        m_size = 0;
    }

    inline void set_active(bool active) noexcept {
        m_active = active;
    }

    inline void set_text(std::string_view text) noexcept {
        m_data = text;
    }

    [[nodiscard]]
    const std::string& text() const noexcept {
        return m_data;
    }

    [[nodiscard]]
    std::string& text() noexcept {
        return m_data;
    }

    [[nodiscard]]
    uint32_t size() const noexcept {
        return m_size;
    }

    [[nodiscard]]
    inline bool active() const noexcept {
        return m_active;
    }

private:
    void remove_last() noexcept {
        if (m_data.empty()) return;

        uint32_t new_size = m_data.size();
        while((static_cast<uint8_t>(m_data[--new_size]) & 0xC0u) == 0x80u);

        m_data.resize(new_size);
        m_size -= 1;
    }

private:
    std::function<bool(uint32_t)> m_filter_function = nullptr;
    sge::Timer m_backspace_timer = sge::Timer::from_seconds(0.5f, sge::TimerMode::Once);
    std::string m_data;
    uint32_t m_size = 0;
    uint32_t m_max_characters = UINT32_MAX;
    bool m_active = false;
};

#endif