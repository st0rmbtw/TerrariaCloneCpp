#ifndef UI_TEXT_INPUT_DATA_HPP_
#define UI_TEXT_INPUT_DATA_HPP_

#include <charconv>
#include <cstdint>
#include <functional>
#include <string_view>

#include <SGE/utils/text.hpp>
#include <SGE/utils/utf8.hpp>
#include <SGE/time/timer.hpp>

class TextInputData {
public:
    TextInputData(uint32_t max_characters = UINT32_MAX) : m_max_characters(max_characters) {}
    TextInputData(std::function<bool(uint32_t)> filter) : m_filter_function(std::move(filter)) {}

    void update() noexcept;

    void clear() noexcept {
        m_data.clear();
        m_cursor_position = 0;
        m_size = 0;
    }

    inline void set_filter(std::function<bool(uint32_t)> callback) noexcept {
        m_filter_function = std::move(callback);
    }

    inline void set_active(bool active) noexcept {
        m_active = active;
    }

    inline void set_bar_visible(bool visible) noexcept {
        m_bar_visible = visible;
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

    template <typename T>
    std::from_chars_result from_chars(T& output, int base = 10) {
        return std::from_chars(m_data.data(), m_data.data() + m_data.size(), output, base);
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
    inline bool bar_visible() const noexcept {
        return m_bar_visible;
    }

    [[nodiscard]]
    inline bool empty() const noexcept {
        return m_data.empty();
    }

    [[nodiscard]]
    inline size_t display_begin() const noexcept {
        return m_display_begin;
    }

    [[nodiscard]]
    inline size_t cursor_position() const noexcept {
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
        if (m_display_begin > 0) {
            const uint8_t bytes = sge::count_utf8_char_bytes_from_end(m_data.data(), m_display_begin);
            if (m_display_begin >= bytes) {
                m_display_begin -= bytes;
            } else {
                m_display_begin = 0;
            }
        }
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
    sge::Timer m_backspace_timer = sge::Timer::from_seconds(1.0f / 20.0f, sge::TimerMode::Repeating);
    std::string m_data;
    size_t m_cursor_position = 0;
    size_t m_display_begin = 0;
    uint32_t m_size = 0;
    uint32_t m_max_characters = UINT32_MAX;
    float m_hold_delay = 0.0f;
    bool m_active = false;
    bool m_bar_visible = false;
};

#endif // UI_TEXT_INPUT_DATA_HPP_