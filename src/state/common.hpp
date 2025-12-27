#pragma once

#include "SGE/utils/utf8.hpp"
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
    TextInputData(uint32_t max_characters = UINT32_MAX) : m_max_characters(max_characters) {
    }
    TextInputData(std::function<bool(uint32_t)> filter) noexcept :
        m_filter_function(std::move(filter)) {};

    void update() noexcept;

    void clear() noexcept {
        m_data.clear();
        m_cursor_position = 0;
        m_size = 0;
    }

    inline void set_active(bool active) noexcept {
        m_active = active;
    }

    inline void set_text(std::string_view text) noexcept {
        m_data = text;
        m_cursor_position = text.size();
        m_size = sge::count_utf8_codepoints(text.data(), text.size());
    }

    inline void set_window_begin(uint32_t begin) noexcept {
        m_display_begin = begin;
    }

    void add_char(char c) {
        m_data.push_back(c);
        m_cursor_position += 1;
        m_size += 1;
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
        return m_data.size();
    }

    [[nodiscard]]
    inline bool active() const noexcept {
        return m_active;
    }

    [[nodiscard]]
    inline bool empty() const noexcept {
        return m_data.empty();
    }

    [[nodiscard]]
    inline uint32_t display_begin() const noexcept {
        return m_display_begin;
    }

    [[nodiscard]]
    inline uint32_t cursor_position() const noexcept {
        return m_cursor_position;
    }

private:
    void clear_before_cursor() {
        while (m_cursor_position > 0) {
            remove_before_cursor();
        }
    }

    void remove_before_cursor() noexcept {
        if (m_data.empty()) return;
        if (m_cursor_position == 0) return;

        const uint8_t count = sge::count_utf8_char_bytes_from_end(m_data.data(), m_cursor_position);

        m_data.erase(m_cursor_position - count, count);
        m_size -= 1;
        m_cursor_position -= count;
    }

    void move_cursor_left() noexcept {
        if (m_cursor_position == 0) return;

        const uint8_t count = sge::count_utf8_char_bytes_from_end(m_data.data(), m_cursor_position);
        m_cursor_position -= count;
    }

    void move_cursor_right() noexcept {
        if (m_cursor_position >= m_data.size()) return;
        
        const uint8_t count = sge::count_utf8_char_bytes(m_data.data()[m_cursor_position]);
        m_cursor_position += count;
    }

    void move_cursor_start() noexcept {
        m_cursor_position = 0;
    }

    void move_cursor_end() noexcept {
        m_cursor_position = m_data.size();
    }

private:
    std::function<bool(uint32_t)> m_filter_function = nullptr;
    sge::Timer m_backspace_timer = sge::Timer::from_seconds(0.5f, sge::TimerMode::Once);
    std::string m_data;
    uint32_t m_size = 0;
    uint32_t m_max_characters = UINT32_MAX;
    uint32_t m_cursor_position = 0;
    uint32_t m_display_begin = 0;
    bool m_active = false;
};

#endif