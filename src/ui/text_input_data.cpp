#include "text_input_data.hpp"

#include <SGE/input.hpp>
#include <SGE/time/time.hpp>

static constexpr float HOLD_DELAY = 0.5f;

void TextInputData::update() noexcept {
    if (!m_active) return;

    for (uint32_t codepoint : sge::Input::CodePoints()) {
        if (m_size >= m_max_characters)
            break;

        uint8_t buffer[4];
        const uint8_t cplen = sge::utf32_codepoint_to_ut8(codepoint, buffer);

        if (m_filter_function != nullptr && !m_filter_function(codepoint))
            continue;

        m_data.insert(m_data.begin() + m_cursor_position, buffer, buffer + cplen);
        m_size += 1;
        m_cursor_position += cplen;
        m_bar_visible = true;
    }

    bool reset_timer = true;

    if (sge::Input::JustPressed(sge::Key::Backspace, sge::Modifier::Control)) {
        clear_before_cursor();
    } else if (sge::Input::JustPressed(sge::Key::Backspace)) {
        remove_before_cursor();
        m_bar_visible = true;
    } else if (sge::Input::Pressed(sge::Key::Backspace)) {
        reset_timer = false;
        m_bar_visible = true;

        if (m_hold_delay < HOLD_DELAY) {
            m_hold_delay += sge::Time::DeltaSeconds();
        } else {
            if (m_backspace_timer.tick(sge::Time::Delta()).finished()) {
                remove_before_cursor();
            }
        }
    }

    if (sge::Input::JustPressed(sge::Key::Home) || sge::Input::JustPressed(sge::Key::ArrowLeft, sge::Modifier::Control)) {
        move_cursor_start();
    } else if (sge::Input::JustPressed(sge::Key::ArrowLeft)) {
        move_cursor_left();
        m_bar_visible = true;
    } else if (sge::Input::Pressed(sge::Key::ArrowLeft)) {
        reset_timer = false;
        m_bar_visible = true;

        if (m_hold_delay < HOLD_DELAY) {
            m_hold_delay += sge::Time::DeltaSeconds();
        } else {
            if (m_backspace_timer.tick(sge::Time::Delta()).finished()) {
                move_cursor_left();
            }
        }
    }

    if (sge::Input::JustPressed(sge::Key::End) || sge::Input::JustPressed(sge::Key::ArrowRight, sge::Modifier::Control)) {
        move_cursor_end();
    } else if (sge::Input::JustPressed(sge::Key::ArrowRight)) {
        m_bar_visible = true;
        move_cursor_right();
    } else if (sge::Input::Pressed(sge::Key::ArrowRight)) {
        reset_timer = false;
        m_bar_visible = true;

        if (m_hold_delay < HOLD_DELAY) {
            m_hold_delay += sge::Time::DeltaSeconds();
        } else {
            if (m_backspace_timer.tick(sge::Time::Delta()).finished()) {
                move_cursor_right();
            }
        }
    }

    if (reset_timer) {
        m_backspace_timer.reset();
        m_hold_delay = 0.0f;
    }
}