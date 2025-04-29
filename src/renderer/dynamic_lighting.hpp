#ifndef _RENDERER_DYNAMIC_LIGHTING_HPP_
#define _RENDERER_DYNAMIC_LIGHTING_HPP_

#include <cstdint>

#include <SGE/renderer/camera.hpp>
#include <SGE/renderer/renderer.hpp>
#include <SGE/engine.hpp>

#include "../world/world.hpp"
#include "../utils/thread_pool.hpp"

class IDynamicLighting {
public:
    virtual void update(WorldData& world) = 0;
    virtual void compute_light(const sge::Camera& camera, const World& world) = 0;

    virtual void destroy() = 0;
};

class DynamicLighting : public IDynamicLighting {
public:
    DynamicLighting(const WorldData& world, LLGL::Texture* light_texture);

    void update(WorldData&) override {};
    void compute_light(const sge::Camera& camera, const World& world) override;
    void destroy() override {
        m_thread_pool.stop();
    };

private:
    ThreadPool m_thread_pool;

    std::vector<sge::IRect> m_dense_areas;
    std::vector<sge::IRect> m_nondense_areas;

    LightMap m_dynamic_lightmap;

    LLGL::Texture* m_light_texture = nullptr;
    sge::Renderer* m_renderer = nullptr;
};

struct LightMapChunk {
    LLGL::Texture* texture;
    LLGL::Buffer* vertex_buffer;
};

class AcceleratedDynamicLighting : public IDynamicLighting {
public:
    AcceleratedDynamicLighting(const WorldData& world, LLGL::Texture* light_texture);

    void update(WorldData& world) override;
    void compute_light(const sge::Camera& camera, const World& world) override;

    void destroy() override;
private:
    void init_textures(const WorldData& world);

    void init_pipeline();

    void update_tile_texture(WorldData& world);
private:
    std::unordered_map<glm::uvec2, LightMapChunk> m_lightmap_chunks;

    sge::Renderer* m_renderer = nullptr;
    void (*m_dispatch_func)(LLGL::CommandBuffer*, uint32_t) = nullptr;

    LLGL::Buffer* m_light_buffer = nullptr;
    LLGL::Texture* m_tile_texture = nullptr;
    LLGL::ResourceHeap* m_light_init_resource_heap = nullptr;
    LLGL::ResourceHeap* m_light_blur_resource_heap = nullptr;

    LLGL::Texture* m_light_texture = nullptr;

    LLGL::PipelineState* m_light_set_light_sources_pipeline = nullptr;
    LLGL::PipelineState* m_light_vertical_pipeline = nullptr;
    LLGL::PipelineState* m_light_horizontal_pipeline = nullptr;

    uint32_t m_workgroup_size = 16;
};

#endif