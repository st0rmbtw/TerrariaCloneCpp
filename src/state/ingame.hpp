#pragma once

#ifndef STATE_INGAME_HPP_
#define STATE_INGAME_HPP_

#include <SGE/renderer/camera.hpp>
#include <SGE/time/timer.hpp>
#include <SGE/types/order.hpp>

#include "../player/player.hpp"
#include "../background.hpp"
#include "../renderer/renderer.hpp"

#include "common.hpp"

#include "base.hpp"

class InGameState : public BaseState {
public:
    InGameState(const std::shared_ptr<sge::Renderer2D>& renderer, uint8_t samples, WorldData world);
    void Render(const std::shared_ptr<sge::GlfwWindow>& window) override;
    void PreUpdate() override;
    void Update() override;
    void OnPreFixedUpdate() override;
    void FixedUpdate() override;
    void OnWindowSizeChanged(LLGL::Extent2D size) override {
        m_camera.set_viewport(size);

        m_world.chunk_manager().manage_chunks(m_world.data(), m_camera);

        Background::UpdateInGame(m_camera, m_world);
    }

    void OnFramebufferSizeChanged(LLGL::Extent2D size) override {
        m_renderer->ResizeTextures(size);
    }

    BaseState* GetNextState() override;

private:
    void update_ui() noexcept;
    void draw_ui() noexcept;
    void draw_inventory() noexcept;
    void draw_cursor() noexcept;

    inline void select_hotbar_slot(Inventory& inventory, uint8_t slot) {
        if (slot == inventory.selected_slot()) return;

        m_hotbar_slot_anim = 0.0f;
        m_previous_selected_slot = inventory.selected_slot();
        inventory.set_selected_slot(slot);
    }

    glm::vec2 camera_follow_player() noexcept;

    #if DEBUG_TOOLS
    glm::vec2 camera_free() noexcept;
    #endif

private:
    std::shared_ptr<GameRenderer> m_renderer;

    Player m_player;
    World m_world;
    sge::Camera m_camera;
    Cursor m_cursor;

    std::string m_ui_fps_text;
    sge::Timer m_fps_update_timer;

    sge::Timer m_light_update_timer = sge::Timer::from_seconds(1.0 / 120.0, sge::TimerMode::Repeating);

    std::vector<Light> m_lights;

    float m_hotbar_slot_anim = 1.0f;

    uint8_t m_previous_selected_slot = 0;

    bool m_free_camera = false;
    bool m_ui_show_fps = false;
    bool m_show_extra_ui = false;
};

#endif