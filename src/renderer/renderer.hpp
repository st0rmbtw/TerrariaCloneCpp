#pragma once

#ifndef RENDERER_HPP_
#define RENDERER_HPP_

#include <LLGL/SwapChain.h>
#include <LLGL/RenderSystem.h>
#include <LLGL/RenderingDebugger.h>
#include <LLGL/Format.h>

#include <SGE/types/sprite.hpp>
#include <SGE/types/nine_patch.hpp>
#include <SGE/types/order.hpp>
#include <SGE/types/rich_text.hpp>
#include <SGE/renderer/camera.hpp>
#include <SGE/types/color.hpp>
#include <SGE/types/blend_mode.hpp>
#include <SGE/renderer/renderer.hpp>
#include <SGE/types/binding_layout.hpp>

#include "../types/background_layer.hpp"

#include "../particles.hpp"

#include "../world/world_data.hpp"
#include "../world/world.hpp"

#include "background_renderer.hpp"
#include "particle_renderer.hpp"
#include "world_renderer.hpp"

class GameRenderer {
public:
    GameRenderer(const std::shared_ptr<sge::Renderer>& renderer);
    ~GameRenderer();

    void InitWorldRenderer(const WorldData& world);
    void ResizeTextures(LLGL::Extent2D resolution);

    void Begin(const sge::Camera& camera, World& world);
    void Render(const std::shared_ptr<sge::GlfwWindow>& window, const sge::Camera& camera, const World& world);

    void UpdateLight();

    uint32_t DrawSprite(const sge::Sprite& sprite, sge::Order order = {});
    uint32_t DrawSpriteWorld(const sge::Sprite& sprite, sge::Order order = {});

    uint32_t DrawAtlasSprite(const sge::TextureAtlasSprite& sprite, sge::Order order = {});
    uint32_t DrawAtlasSpriteWorld(const sge::TextureAtlasSprite& sprite, sge::Order order = {});

    uint32_t DrawAtlasSpriteWorldPremultiplied(const sge::TextureAtlasSprite& sprite, sge::Order order = {});

    uint32_t DrawSpriteUI(const sge::Sprite& sprite, sge::Order order = {});
    uint32_t DrawAtlasSpriteUI(const sge::TextureAtlasSprite& sprite, sge::Order order = {});

    uint32_t DrawNinePatchUI(const sge::NinePatch& ninepatch, sge::Order order = {});

    uint32_t DrawText(const sge::RichTextSection* sections, size_t size, const glm::vec2& position, const sge::Font& font, sge::Order order = {});
    uint32_t DrawTextUI(const sge::RichTextSection* sections, size_t size, const glm::vec2& position, const sge::Font& font, sge::Order order = {});

    template <size_t L>
    inline uint32_t DrawText(const sge::RichText<L>& text, const glm::vec2& position, const sge::Font& font, sge::Order order = {}) {
        return DrawText(text.data(), L, position, font, order);
    }

    template <size_t L>
    inline uint32_t DrawTextUI(const sge::RichText<L>& text, const glm::vec2& position, const sge::Font& font, sge::Order order = {}) {
        return DrawTextUI(text.data(), L, position, font, order);
    }

    inline uint32_t DrawChar(char ch, const glm::vec2& position, float size, const sge::LinearRgba& color, const sge::Font& font, sge::Order order = {}) {
        char text[] = {ch, '\0'};
        const sge::RichTextSection section(text, color, size);
        return DrawText(&section, 1, position, font, order);
    }
    inline uint32_t DrawCharUI(char ch, const glm::vec2& position, float size, const sge::LinearRgba& color, const sge::Font& font, sge::Order order = {}) {
        char text[] = {ch, '\0'};
        const sge::RichTextSection section(text, color, size);
        return DrawTextUI(&section, 1, position, font, order);
    }

    void DrawBackground(const BackgroundLayer& layer);
    void DrawParticle(const glm::vec2& position, const glm::quat& rotation, float scale, Particle::Type type, uint8_t variant, sge::Order order = {}, bool world = false);

    void BeginOrderMode(int order, bool advance) noexcept;

    inline void BeginOrderMode(int order = -1) noexcept {
        BeginOrderMode(order, true);
    }

    inline void BeginOrderMode(bool advance) noexcept {
        BeginOrderMode(-1, advance);
    }

    void EndOrderMode() noexcept;

    void BeginBlendMode(sge::BlendMode blend_mode) noexcept;
    void EndBlendMode() noexcept;

    [[nodiscard]] uint32_t GetMainOrderIndex();
    [[nodiscard]] uint32_t GetWorldOrderIndex();
    [[nodiscard]] LLGL::Buffer* ChunkVertexBuffer();

    [[nodiscard]]
    const std::shared_ptr<sge::Renderer> GetRenderer() const {
        return m_renderer;
    }

private:
    BackgroundRenderer m_background_renderer;
    
    std::shared_ptr<sge::Renderer> m_renderer = nullptr;

    std::unique_ptr<sge::Batch> m_main_batch = nullptr;
    std::unique_ptr<sge::Batch> m_world_batch = nullptr;
    std::unique_ptr<sge::Batch> m_ui_batch = nullptr;

    sge::Rect m_camera_frustums[2];
    sge::Rect m_ui_frustum;

    ParticleRenderer m_particle_renderer;
    WorldRenderer m_world_renderer;

    LLGL::ResourceHeap* m_resource_heap = nullptr;

    LLGL::Buffer* m_chunk_vertex_buffer = nullptr;

    uint32_t m_postprocess_pipeline_id = -1;
    LLGL::Buffer* m_postprocess_vertex_buffer = nullptr;
    LLGL::Buffer* m_postprocess_uniform_buffer = nullptr;

    bool m_update_light = false;
};

#endif