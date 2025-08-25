#pragma once

#ifndef RENDERER_PARTICLE_RENDERER_HPP_
#define RENDERER_PARTICLE_RENDERER_HPP_

#include <LLGL/Buffer.h>
#include <LLGL/BufferArray.h>
#include <LLGL/PipelineState.h>

#include <SGE/types/texture_atlas.hpp>
#include <SGE/renderer/renderer.hpp>
#include <SGE/types/order.hpp>

#include "../particles.hpp"

#include "types.hpp"

class ParticleRenderer {
public:
    void init();
    void render();
    void render_world();
    void compute();
    void terminate();
    void prepare();
    void reset();

    void draw_particle(const glm::vec2& position, const glm::quat& rotation, float scale, Particle::Type type, uint8_t variant, sge::Order order);
    void draw_particle_world(const glm::vec2& position, const glm::quat& rotation, float scale, Particle::Type type, uint8_t variant, sge::Order order);

private:
    sge::Renderer* m_renderer = nullptr;

    LLGL::PipelineState* m_pipeline = nullptr;
    LLGL::PipelineState* m_compute_pipeline = nullptr;
    LLGL::ResourceHeap* m_resource_heap = nullptr;
    LLGL::ResourceHeap* m_compute_resource_heap = nullptr;

    LLGL::BufferArray* m_buffer_array = nullptr;
    LLGL::Buffer* m_instance_buffer = nullptr;
    LLGL::Buffer* m_vertex_buffer = nullptr;

    LLGL::Buffer* m_position_buffer = nullptr;
    LLGL::Buffer* m_rotation_buffer = nullptr;
    LLGL::Buffer* m_scale_buffer = nullptr;

    LLGL::Buffer* m_transform_buffer = nullptr;

    ParticleInstance* m_instance_buffer_data = nullptr;
    ParticleInstance* m_instance_buffer_data_ptr = nullptr;

    ParticleInstance* m_instance_buffer_data_world = nullptr;
    ParticleInstance* m_instance_buffer_data_world_ptr = nullptr;

    glm::vec2* m_position_buffer_data = nullptr;
    glm::vec2* m_position_buffer_data_ptr = nullptr;

    glm::quat* m_rotation_buffer_data = nullptr;
    glm::quat* m_rotation_buffer_data_ptr = nullptr;

    float* m_scale_buffer_data = nullptr;
    float* m_scale_buffer_data_ptr = nullptr;

    sge::TextureAtlas m_atlas;

    uint32_t m_particle_count;
    uint32_t m_world_particle_count;
    uint32_t m_particle_id;
    bool m_is_metal;
};

#endif