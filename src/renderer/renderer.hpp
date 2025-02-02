#ifndef RENDERER_HPP
#define RENDERER_HPP

#pragma once

#include <LLGL/SwapChain.h>
#include <LLGL/RenderSystem.h>
#include <LLGL/RenderingDebugger.h>
#include <LLGL/Format.h>

#include "../types/sprite.hpp"
#include "../types/nine_patch.hpp"
#include "../types/background_layer.hpp"
#include "../types/order.hpp"
#include "../types/rich_text.hpp"
#include "../assets.hpp"
#include "../particles.hpp"

#include "../world/world_data.hpp"
#include "../world/world.hpp"

#include "camera.h"

namespace GameRenderer {
    bool Init(const LLGL::Extent2D& resolution);
    void InitWorldRenderer(const WorldData& world);
    void ResizeTextures(LLGL::Extent2D resolution);

    void Begin(const Camera& camera, WorldData& world);
    void Render(const Camera& camera, const World& world);

    void UpdateLight();

    void DrawSprite(const Sprite& sprite, Order order = {});
    void DrawSpriteWorld(const Sprite& sprite, Order order = {});
    
    void DrawAtlasSprite(const TextureAtlasSprite& sprite, Order order = {});
    void DrawAtlasSpriteWorld(const TextureAtlasSprite& sprite, Order order = {});

    void DrawSpriteUI(const Sprite& sprite, Order order = -1);
    void DrawAtlasSpriteUI(const TextureAtlasSprite& sprite, Order order = -1);

    void DrawNinePatchUI(const NinePatch& ninepatch, Order order = -1);

    void DrawText(const RichTextSection* sections, size_t size, const glm::vec2& position, FontAsset font, bool is_ui = false, Order order = -1);

    template <size_t L>
    inline void DrawText(const RichText<L>& text, const glm::vec2& position, FontAsset font, Order order = -1) {
        DrawText(text.sections().data(), L, position, font, false, order);
    }

    template <size_t L>
    inline void DrawTextUI(const RichText<L>& text, const glm::vec2& position, FontAsset font, Order order = -1) {
        DrawText(text.sections().data(), L, position, font, true, order);
    }

    inline void DrawChar(char ch, const glm::vec2& position, float size, const glm::vec3& color, FontAsset font, Order order = -1) {
        char text[] = {ch, '\0'};
        const RichTextSection section(text, size, color);
        DrawText(&section, 1, position, font, false, order);
    }
    inline void DrawCharUI(char ch, const glm::vec2& position, float size, const glm::vec3& color, FontAsset font, Order order = -1) {
        char text[] = {ch, '\0'};
        const RichTextSection section(text, size, color);
        DrawText(&section, 1, position, font, true, order);
    }

    void DrawBackground(const BackgroundLayer& layer);
    void DrawParticle(const glm::vec2& position, const glm::quat& rotation, float scale, Particle::Type type, uint8_t variant, Order order = -1, bool world = false);

    void BeginOrderMode(Order order = -1);
    void EndOrderMode(Order order = -1);

    void Terminate();

    [[nodiscard]] uint32_t GetMainOrderIndex();
    [[nodiscard]] uint32_t GetWorldOrderIndex();
    [[nodiscard]] LLGL::Buffer* ChunkVertexBuffer();
};

#endif