#ifndef RENDERER_HPP
#define RENDERER_HPP

#pragma once

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

#include "../types/background_layer.hpp"

#include "../particles.hpp"

#include "../world/world_data.hpp"
#include "../world/world.hpp"

namespace GameRenderer {
    bool Init(const LLGL::Extent2D& resolution);
    void InitWorldRenderer(const WorldData& world);
    void ResizeTextures(LLGL::Extent2D resolution);

    void Begin(const sge::Camera& camera, WorldData& world);
    void Render(const sge::Camera& camera, const World& world);

    void UpdateLight();

    uint32_t DrawSprite(const sge::Sprite& sprite, sge::Order order = {});
    uint32_t DrawSpriteWorld(const sge::Sprite& sprite, sge::Order order = {});
    
    uint32_t DrawAtlasSprite(const sge::TextureAtlasSprite& sprite, sge::Order order = {});
    uint32_t DrawAtlasSpriteWorld(const sge::TextureAtlasSprite& sprite, sge::Order order = {});

    uint32_t DrawSpriteUI(const sge::Sprite& sprite, sge::Order order = {});
    uint32_t DrawAtlasSpriteUI(const sge::TextureAtlasSprite& sprite, sge::Order order = {});

    uint32_t DrawNinePatchUI(const sge::NinePatch& ninepatch, sge::Order order = {});

    uint32_t DrawText(const sge::RichTextSection* sections, size_t size, const glm::vec2& position, const sge::Font& font, sge::Order order = {});
    uint32_t DrawTextUI(const sge::RichTextSection* sections, size_t size, const glm::vec2& position, const sge::Font& font, sge::Order order = {});

    template <size_t L>
    inline uint32_t DrawText(const sge::RichText<L>& text, const glm::vec2& position, const sge::Font& font, sge::Order order = {}) {
        return DrawText(text.sections().data(), L, position, font, order);
    }

    template <size_t L>
    inline uint32_t DrawTextUI(const sge::RichText<L>& text, const glm::vec2& position, const sge::Font& font, sge::Order order = {}) {
        return DrawTextUI(text.sections().data(), L, position, font, order);
    }

    inline uint32_t DrawChar(char ch, const glm::vec2& position, float size, const sge::LinearRgba& color, const sge::Font& font, sge::Order order = {}) {
        char text[] = {ch, '\0'};
        const sge::RichTextSection section(text, size, color);
        return DrawText(&section, 1, position, font, order);
    }
    inline uint32_t DrawCharUI(char ch, const glm::vec2& position, float size, const sge::LinearRgba& color, const sge::Font& font, sge::Order order = {}) {
        char text[] = {ch, '\0'};
        const sge::RichTextSection section(text, size, color);
        return DrawTextUI(&section, 1, position, font, order);
    }

    void DrawBackground(const BackgroundLayer& layer);
    void DrawParticle(const glm::vec2& position, const glm::quat& rotation, float scale, Particle::Type type, uint8_t variant, sge::Order order = {}, bool world = false);

    void BeginOrderMode(int order, bool advance);
    inline void BeginOrderMode(int order = -1) { BeginOrderMode(order, true); }
    inline void BeginOrderMode(bool advance) { BeginOrderMode(-1, advance); }
    void EndOrderMode();

    void Terminate();

    [[nodiscard]] uint32_t GetMainOrderIndex();
    [[nodiscard]] uint32_t GetWorldOrderIndex();
    [[nodiscard]] LLGL::Buffer* ChunkVertexBuffer();
};

#endif