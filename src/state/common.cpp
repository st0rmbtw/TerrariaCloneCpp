#include "common.hpp"

#include <SGE/time/time.hpp>

static constexpr float MIN_CURSOR_SCALE = 1.2f;
static constexpr float MAX_CURSOR_SCALE = MIN_CURSOR_SCALE + 0.1f;

void Cursor::Update(glm::vec2 mouse_position) {
    if (m_anim_progress >= 1.0f) {
        m_anim_dir = AnimationDirection::Backward;
    } else if (m_anim_progress <= 0.0f) {
        m_anim_dir = AnimationDirection::Forward;
    }

    switch (m_anim_dir) {
    case AnimationDirection::Backward:
        m_anim_progress -= 2.0f * sge::Time::DeltaSeconds();
        break;
    case AnimationDirection::Forward:
        m_anim_progress += 2.0f * sge::Time::DeltaSeconds();
        break;
    }

    m_anim_progress = glm::clamp(m_anim_progress, 0.0f, 1.0f);

    const float scale = MIN_CURSOR_SCALE + m_anim_progress * (MAX_CURSOR_SCALE - MIN_CURSOR_SCALE);
    m_scale = scale;

    m_background.set_position(mouse_position);
    m_foreground.set_position(mouse_position + glm::vec2(3.0f));

    m_background.set_scale(glm::vec2(scale));
    m_foreground.set_scale(glm::vec2(scale));

    m_foreground.set_color(m_foreground_color * (0.7f + 0.3f * m_anim_progress));
}