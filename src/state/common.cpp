#include "common.hpp"

#include <SGE/utils/utf8.hpp>
#include <SGE/time/time.hpp>
#include <SGE/input.hpp>

static constexpr float MIN_CURSOR_SCALE = 1.2f;
static constexpr float MAX_CURSOR_SCALE = MIN_CURSOR_SCALE + 0.1f;

void Cursor::Update(glm::vec2 mouse_position) {
    m_animation.tick(sge::Time::DeltaSeconds());

    const float scale = MIN_CURSOR_SCALE + m_animation.progress() * (MAX_CURSOR_SCALE - MIN_CURSOR_SCALE);
    m_scale = scale;

    m_background.set_position(mouse_position);
    m_foreground.set_position(mouse_position + glm::vec2(3.0f));

    m_background.set_scale(glm::vec2(scale));
    m_foreground.set_scale(glm::vec2(scale));

    m_foreground.set_color(m_foreground_color * (0.7f + 0.3f * m_animation.progress()));
}

void TextInputData::update() noexcept {
    if (!m_active) return;

    for (uint32_t codepoint : sge::Input::CodePoints()) {
        if (m_size >= m_max_characters)
            break;

        uint8_t buffer[4];
        const uint8_t cplen = sge::utf32_codepoint_to_ut8(codepoint, buffer);

        if (m_filter_function != nullptr && !m_filter_function(codepoint))
            continue;

        m_data.insert(m_data.end(), buffer, buffer + cplen);
        m_size += 1;
    }

    if (sge::Input::JustPressed(sge::Key::Backspace, sge::Modifier::Control)) {
        clear();
    } else if (sge::Input::JustPressed(sge::Key::Backspace)) {
        remove_last();
    } else if (sge::Input::Pressed(sge::Key::Backspace)) {
        if (m_backspace_timer.tick(sge::Time::Delta()).finished()) {
            remove_last();
        }
    } else {
        m_backspace_timer.reset();
    }
}