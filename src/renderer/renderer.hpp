#ifndef RENDERER_HPP
#define RENDERER_HPP

#pragma once

#include <LLGL/SwapChain.h>
#include <LLGL/RenderSystem.h>
#include <LLGL/RenderingDebugger.h>
#include <LLGL/Format.h>

#include "../engine/types/sprite.hpp"
#include "../engine/types/nine_patch.hpp"
#include "../engine/types/order.hpp"
#include "../engine/types/rich_text.hpp"
#include "../engine/renderer/camera.hpp"

#include "../types/background_layer.hpp"

#include "../assets.hpp"
#include "../particles.hpp"

#include "../world/world_data.hpp"
#include "../world/world.hpp"

namespace GameRenderer {
    bool Init(const LLGL::Extent2D& resolution);
    void InitWorldRenderer(const WorldData& world);
    void ResizeTextures(LLGL::Extent2D resolution);

    void Begin(const Camera& camera, WorldData& world);
    void Render(const Camera& camera, const World& world);

    void UpdateLight();

    uint32_t DrawSprite(const Sprite& sprite, Order order = {});
    uint32_t DrawSpriteWorld(const Sprite& sprite, Order order = {});
    
    uint32_t DrawAtlasSprite(const TextureAtlasSprite& sprite, Order order = {});
    uint32_t DrawAtlasSpriteWorld(const TextureAtlasSprite& sprite, Order order = {});

    uint32_t DrawSpriteUI(const Sprite& sprite, Order order = -1);
    uint32_t DrawAtlasSpriteUI(const TextureAtlasSprite& sprite, Order order = -1);

    uint32_t DrawNinePatchUI(const NinePatch& ninepatch, Order order = -1);

    uint32_t DrawText(const RichTextSection* sections, size_t size, const glm::vec2& position, FontAsset font, bool is_ui = false, Order order = -1);

    template <size_t L>
    inline uint32_t DrawText(const RichText<L>& text, const glm::vec2& position, FontAsset font, Order order = -1) {
        return DrawText(text.sections().data(), L, position, font, false, order);
    }

    template <size_t L>
    inline uint32_t DrawTextUI(const RichText<L>& text, const glm::vec2& position, FontAsset font, Order order = -1) {
        return DrawText(text.sections().data(), L, position, font, true, order);
    }

    inline uint32_t DrawChar(char ch, const glm::vec2& position, float size, const glm::vec3& color, FontAsset font, Order order = -1) {
        char text[] = {ch, '\0'};
        const RichTextSection section(text, size, color);
        return DrawText(&section, 1, position, font, false, order);
    }
    inline uint32_t DrawCharUI(char ch, const glm::vec2& position, float size, const glm::vec3& color, FontAsset font, Order order = -1) {
        char text[] = {ch, '\0'};
        const RichTextSection section(text, size, color);
        return DrawText(&section, 1, position, font, true, order);
    }

    void DrawBackground(const BackgroundLayer& layer);
    void DrawParticle(const glm::vec2& position, const glm::quat& rotation, float scale, Particle::Type type, uint8_t variant, Order order = -1, bool world = false);

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