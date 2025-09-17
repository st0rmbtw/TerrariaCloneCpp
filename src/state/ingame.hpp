#pragma once

#ifndef STATE_INGAME_HPP_
#define STATE_INGAME_HPP_

#include <SGE/renderer/camera.hpp>
#include <SGE/time/timer.hpp>
#include <SGE/types/order.hpp>

#include "../player/player.hpp"
#include "../background.hpp"

#include "common.hpp"

#include "base.hpp"

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
    Player m_player;
    World m_world;
    sge::Camera m_camera;
    Cursor m_cursor;

    std::string m_ui_fps_text;
    sge::Timer m_fps_update_timer;

    std::vector<Light> m_lights;

    float m_hotbar_slot_anim = 1.0f;

    uint8_t m_previous_selected_slot = 0;

    bool m_free_camera = false;
    bool m_ui_show_fps = false;
    bool m_show_extra_ui = false;
};

#endif