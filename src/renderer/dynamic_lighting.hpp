#pragma once

#ifndef RENDERER_DYNAMIC_LIGHTING_HPP_
#define RENDERER_DYNAMIC_LIGHTING_HPP_

#include <cstdint>

#include <SGE/engine.hpp>
#include <SGE/renderer/camera.hpp>
#include <SGE/renderer/renderer.hpp>
#include <SGE/utils/containers/heaparray.hpp>

#include "../world/world.hpp"

class IDynamicLighting {
public:
    virtual void update(World& world) = 0;
    virtual void compute_light(const sge::Camera& camera, const World& world) = 0;

    virtual void set_light_texture(const sge::Ref<LLGL::Texture>&) noexcept {}

    virtual ~IDynamicLighting() = default;
};

class DynamicLighting : public IDynamicLighting {
public:
    DynamicLighting(std::shared_ptr<sge::Renderer> renderer, const WorldData& world, sge::Ref<LLGL::Texture> light_texture);

    void update(World& world) override;
    void compute_light(const sge::Camera& camera, const World& world) override;
private:
    std::vector<sge::IRect> m_areas;

    std::vector<std::pair<size_t, size_t>> m_indices;

    sge::HeapArray<Color> m_line;

    LightMap m_dynamic_lightmap;

    std::shared_ptr<sge::Renderer> m_renderer = nullptr;
    sge::Ref<LLGL::Texture> m_light_texture = nullptr;
};

class AcceleratedDynamicLighting : public IDynamicLighting {
private:
    struct alignas(16) UniformBuffer {
        glm::uvec2 texture_offset;
        glm::uvec2 blur_offset;
        glm::uvec2 blur_size;
        uint32_t light_count;
    };

public:
    AcceleratedDynamicLighting(std::shared_ptr<sge::Renderer> renderer, const WorldData& world, sge::Ref<LLGL::Texture> light_texture);

    void set_light_texture(const sge::Ref<LLGL::Texture>& light_texture) noexcept override {
        SGE_ASSERT(light_texture.IsValid());
        const auto& context = m_renderer->GetRenderContext()->GetLLGLContext();
        context->WriteResourceHeap(*m_light_blur_resource_heap, 2, { light_texture.Get() });
        context->WriteResourceHeap(*m_light_init_resource_heap, 2, { light_texture.Get() });
        m_light_texture = light_texture;
    }

    void update(World& world) override {
        update_tile_texture(world.data());
    }

    void compute_light(const sge::Camera& camera, const World& world) override;
private:
    void init_textures(const WorldData& world);

    void init_pipeline();

    void update_tile_texture(WorldData& world);
private:
    std::shared_ptr<sge::Renderer> m_renderer = nullptr;

    sge::Unique<LLGL::Buffer> m_light_buffer = nullptr;
    sge::Unique<LLGL::Buffer> m_uniform_buffer = nullptr;
    sge::Unique<LLGL::Texture> m_tile_texture = nullptr;
    sge::Unique<LLGL::ResourceHeap> m_light_init_resource_heap = nullptr;
    sge::Unique<LLGL::ResourceHeap> m_light_blur_resource_heap = nullptr;

    sge::Unique<LLGL::PipelineLayout> m_light_init_pipeline_layout = nullptr;
    sge::Unique<LLGL::PipelineLayout> m_light_blur_pipeline_layout = nullptr;

    sge::Ref<LLGL::Texture> m_light_texture = nullptr;

    sge::Unique<LLGL::PipelineState> m_light_set_light_sources_pipeline = nullptr;
    sge::Unique<LLGL::PipelineState> m_light_vertical_pipeline = nullptr;
    sge::Unique<LLGL::PipelineState> m_light_horizontal_pipeline = nullptr;

    uint32_t m_workgroup_size = 32;
};

#endif
