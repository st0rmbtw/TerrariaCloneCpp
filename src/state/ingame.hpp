#pragma once

#ifndef STATE_PLAY_INGAME_HPP_
#define STATE_PLAY_INGAME_HPP_

#include <SGE/renderer/camera.hpp>
#include <SGE/time/timer.hpp>
#include <SGE/types/order.hpp>

#include "../player/player.hpp"
#include "../background.hpp"

#include "base.hpp"

enum class AnimationDirection : uint8_t {
    Forward = 0,
    Backward = 1
};

class InGameState : public BaseState {
public:
    InGameState();
    void Render() override;
    void PostRender() override;
    void PreUpdate() override;
    void Update() override;
    void FixedUpdate() override;
    void OnWindowSizeChanged(glm::uvec2 size) override {
        m_camera.set_viewport(size);
        m_camera.update();

        m_world.chunk_manager().manage_chunks(m_world.data(), m_camera);

        Background::UpdateInGame(m_camera, m_world);
    }
    BaseState* GetNextState() override;
    ~InGameState();

private:
    void update_ui_cursor() noexcept;
    void update_ui() noexcept;
    void draw_ui() noexcept;
    void draw_inventory() noexcept;
    void draw_cursor() noexcept;

    void draw_item(const glm::vec2& item_size, const glm::vec2& position, const Item& item, sge::Order item_order = {});
    void draw_item_with_stack(const sge::Font& font, const glm::vec2& item_size, float stack_size, const glm::vec2& position, const Item& item, sge::Order item_order = {}, sge::Order stack_order = {});

    glm::vec2 camera_follow_player() noexcept;

    #if DEBUG_TOOLS
    glm::vec2 camera_free() noexcept;
    #endif

private:
    Player m_player;
    World m_world;
    sge::Camera m_camera;
    sge::Sprite m_cursor_foreground;
    sge::Sprite m_cursor_background;

    std::string m_ui_fps_text;
    sge::Timer m_fps_update_timer;

    std::vector<Light> m_lights;
    sge::LinearRgba m_cursor_foreground_color;
    sge::LinearRgba m_cursor_background_color;

    AnimationDirection m_cursor_anim_dir;

    float m_cursor_anim_progress = 0.0f;
    float m_cursor_scale = 1.0f;

    float m_hotbar_slot_anim = 1.0f;

    uint8_t m_previous_selected_slot = 0;

    bool m_free_camera = false;
    bool m_ui_show_fps = false;
    bool m_show_extra_ui = false;
};

#endif