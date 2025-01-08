#ifndef TERRARIA_RENDERER_HPP
#define TERRARIA_RENDERER_HPP

#pragma once

#include <LLGL/SwapChain.h>
#include <LLGL/RenderSystem.h>
#include <LLGL/RenderingDebugger.h>
#include <LLGL/Format.h>

#include "../types/sprite.hpp"
#include "../types/backend.hpp"
#include "../types/background_layer.hpp"
#include "../types/render_layer.hpp"
#include "../types/shader_path.hpp"
#include "../types/depth.hpp"
#include "../types/rich_text.hpp"
#include "../assets.hpp"
#include "../particles.hpp"

#include "../world/world_data.hpp"
#include "../world/world.hpp"

#include "custom_surface.hpp"
#include "camera.h"

struct __attribute__((aligned(16))) ProjectionsUniform {
    glm::mat4 screen_projection_matrix;
    glm::mat4 view_projection_matrix;
    glm::mat4 nonscale_view_projection_matrix;
    glm::mat4 nonscale_projection_matrix;
    glm::mat4 transform_matrix;
    glm::mat4 inv_view_proj_matrix;
    glm::vec2 camera_position;
    glm::vec2 window_size;
    float max_depth;
    float max_world_depth;
};

namespace Renderer {
    bool InitEngine(RenderBackend backend);
    bool Init(GLFWwindow* window, const LLGL::Extent2D& resolution, bool vsync, bool fullscreen);
    void InitWorldRenderer(const WorldData& world);
    void ResizeTextures(LLGL::Extent2D resolution);

    void Begin(const Camera& camera, WorldData& world);
    void Render(const Camera& camera, const World& world);

    void DrawSprite(const Sprite& sprite, RenderLayer render_layer = RenderLayer::Main, Depth depth = {});
    inline void DrawSprite(const Sprite& sprite, Depth depth) {
        DrawSprite(sprite, RenderLayer::Main, depth);
    }
    
    void DrawAtlasSprite(const TextureAtlasSprite& sprite, RenderLayer render_layer = RenderLayer::Main, Depth depth = {});
    inline void DrawAtlasSprite(const TextureAtlasSprite& sprite, Depth depth) {
        DrawAtlasSprite(sprite, RenderLayer::Main, depth);
    }

    void DrawSpriteUI(const Sprite& sprite, Depth depth = -1);
    void DrawAtlasSpriteUI(const TextureAtlasSprite& sprite, Depth depth = -1);

    void DrawText(const RichTextSection* sections, size_t size, const glm::vec2& position, FontAsset font, bool is_ui = false, Depth depth = -1);

    template <size_t L>
    inline void DrawText(const RichText<L>& text, const glm::vec2& position, FontAsset font, Depth depth = -1) {
        DrawText(text.sections().data(), L, position, font, false, depth);
    }

    template <size_t L>
    inline void DrawTextUI(const RichText<L>& text, const glm::vec2& position, FontAsset font, Depth depth = -1) {
        DrawText(text.sections().data(), L, position, font, true, depth);
    }

    inline void DrawChar(char ch, const glm::vec2& position, float size, const glm::vec3& color, FontAsset font, Depth depth = -1) {
        char text[] = {ch, '\0'};
        const RichTextSection section(text, size, color);
        DrawText(&section, 1, position, font, false, depth);
    }
    inline void DrawCharUI(char ch, const glm::vec2& position, float size, const glm::vec3& color, FontAsset font, Depth depth = -1) {
        char text[] = {ch, '\0'};
        const RichTextSection section(text, size, color);
        DrawText(&section, 1, position, font, true, depth);
    }

    void DrawBackground(const BackgroundLayer& layer);
    void DrawParticle(const glm::vec2& position, const glm::quat& rotation, float scale, Particle::Type type, uint8_t variant, Depth depth = -1);

#if DEBUG
    void PrintDebugInfo();
#endif

    void BeginDepth(Depth depth = {});
    void EndDepth();

    Texture CreateTexture(LLGL::TextureType type, LLGL::ImageFormat image_format, LLGL::DataType data_type, uint32_t width, uint32_t height, uint32_t layers, int sampler, const void* data, bool generate_mip_maps = false);
    inline Texture CreateTexture(LLGL::TextureType type, LLGL::ImageFormat image_format, uint32_t width, uint32_t height, uint32_t layers, int sampler, const uint8_t* data, bool generate_mip_maps = false) {
        return CreateTexture(type, image_format, LLGL::DataType::UInt8, width, height, layers, sampler, data, generate_mip_maps);
    }
    inline Texture CreateTexture(LLGL::TextureType type, LLGL::ImageFormat image_format, uint32_t width, uint32_t height, uint32_t layers, int sampler, const int8_t* data, bool generate_mip_maps = false) {
        return CreateTexture(type, image_format, LLGL::DataType::Int8, width, height, layers, sampler, data, generate_mip_maps);
    }
    LLGL::Shader* LoadShader(const ShaderPath& shader_path, const std::vector<ShaderDef>& shader_defs = {}, const std::vector<LLGL::VertexAttribute>& vertex_attributes = {});

    void Terminate();

    [[nodiscard]] const LLGL::RenderSystemPtr& Context();
    [[nodiscard]] LLGL::SwapChain* SwapChain();
    [[nodiscard]] LLGL::CommandBuffer* CommandBuffer();
    [[nodiscard]] LLGL::CommandQueue* CommandQueue();
    [[nodiscard]] const std::shared_ptr<CustomSurface>& Surface();
    [[nodiscard]] LLGL::Buffer* GlobalUniformBuffer();
    [[nodiscard]] RenderBackend Backend();
    [[nodiscard]] uint32_t GetMainDepthIndex();
    [[nodiscard]] uint32_t GetWorldDepthIndex();
    [[nodiscard]] LLGL::Buffer* ChunkVertexBuffer();

#if DEBUG
    [[nodiscard]] LLGL::RenderingDebugger* Debugger();
#endif
};

#endif