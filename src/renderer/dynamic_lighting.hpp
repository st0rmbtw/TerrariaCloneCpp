#pragma once

#ifndef RENDERER_DYNAMIC_LIGHTING_HPP_
#define RENDERER_DYNAMIC_LIGHTING_HPP_

#include <cstdint>

#include <SGE/renderer/camera.hpp>
#include <SGE/renderer/renderer.hpp>
#include <SGE/engine.hpp>

#include "../world/world.hpp"

class IDynamicLighting {
public:
    virtual void update(World& world) = 0;
    virtual void compute_light(const sge::Camera& camera, const World& world) = 0;

    virtual void destroy() = 0;

    virtual void set_light_texture(LLGL::Texture*) noexcept {}

    virtual ~IDynamicLighting() = default;
};

class DynamicLighting : public IDynamicLighting {
public:
    DynamicLighting(const WorldData& world, LLGL::Texture* light_texture);

    void update(World& world) override;
    void compute_light(const sge::Camera& camera, const World& world) override;

    void destroy() override {}

    ~DynamicLighting() override {
        destroy();
    }
private:
    std::vector<sge::IRect> m_areas;

    std::vector<std::pair<size_t, size_t>> m_indices;

    HeapArray<Color> m_line;

    LightMap m_dynamic_lightmap;

    sge::LLGLResource<LLGL::Texture> m_light_texture = nullptr;
    sge::Renderer* m_renderer = nullptr;
};

class AcceleratedDynamicLighting : public IDynamicLighting {
private:
    struct alignas(16) UniformBuffer {
        glm::uvec2 texture_offset;
        glm::uvec2 blur_offset;
        glm::uvec2 blur_size;
    };

public:
    AcceleratedDynamicLighting(const WorldData& world, LLGL::Texture* light_texture);

    void set_light_texture(LLGL::Texture* light_texture) noexcept override {
        SGE_ASSERT(light_texture != nullptr);
        const auto& context = m_renderer->Context();
        context->WriteResourceHeap(*m_light_blur_resource_heap, 2, { light_texture });
        context->WriteResourceHeap(*m_light_init_resource_heap, 2, { light_texture });
        m_light_texture = light_texture;
    }

    void update(World& world) override {
        update_tile_texture(world.data());
    }

    void compute_light(const sge::Camera& camera, const World& world) override;

    void destroy() override;
private:
    void init_textures(const WorldData& world);

    void init_pipeline();

    void update_tile_texture(WorldData& world);
private:
    sge::Renderer* m_renderer = nullptr;

    sge::LLGLResource<LLGL::Buffer> m_light_buffer = nullptr;
    sge::LLGLResource<LLGL::Buffer> m_uniform_buffer = nullptr;
    sge::LLGLResource<LLGL::Texture> m_tile_texture = nullptr;
    sge::LLGLResource<LLGL::ResourceHeap> m_light_init_resource_heap = nullptr;
    sge::LLGLResource<LLGL::ResourceHeap> m_light_blur_resource_heap = nullptr;

    sge::LLGLResource<LLGL::Texture> m_light_texture = nullptr;

    sge::LLGLResource<LLGL::PipelineState> m_light_set_light_sources_pipeline = nullptr;
    sge::LLGLResource<LLGL::PipelineState> m_light_vertical_pipeline = nullptr;
    sge::LLGLResource<LLGL::PipelineState> m_light_horizontal_pipeline = nullptr;

    uint32_t m_workgroup_size = 16;

    bool is_metal = false;
};

#endif
