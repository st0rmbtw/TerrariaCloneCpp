#ifndef _RENDERER_DYNAMIC_LIGHTING_HPP_
#define _RENDERER_DYNAMIC_LIGHTING_HPP_

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

    LLGL::DynamicArray<Color> m_line;

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
    std::unordered_map<glm::uvec2, LightMapChunk> m_lightmap_chunks;

    sge::Renderer* m_renderer = nullptr;

    LLGL::Buffer* m_light_buffer = nullptr;
    LLGL::Texture* m_tile_texture = nullptr;
    LLGL::ResourceHeap* m_light_init_resource_heap = nullptr;
    LLGL::ResourceHeap* m_light_blur_resource_heap = nullptr;

    LLGL::Texture* m_light_texture = nullptr;

    LLGL::PipelineState* m_light_set_light_sources_pipeline = nullptr;
    LLGL::PipelineState* m_light_vertical_pipeline = nullptr;
    LLGL::PipelineState* m_light_horizontal_pipeline = nullptr;

    uint32_t m_workgroup_size = 16;

    bool is_metal = false;
};

#endif
