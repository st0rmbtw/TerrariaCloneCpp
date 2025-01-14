#ifndef RENDERER_PARTICLE_RENDERER_HPP
#define RENDERER_PARTICLE_RENDERER_HPP

#pragma once

#include <LLGL/Buffer.h>
#include <LLGL/BufferArray.h>
#include <LLGL/PipelineState.h>

#include "types.hpp"

#include "../particles.hpp"
#include "../types/texture_atlas.hpp"
#include "../types/depth.hpp"

class ParticleRenderer {
public:
    void init();
    void render();
    void compute();
    void terminate();

    void draw_particle(const glm::vec2& position, const glm::quat& rotation, float scale, Particle::Type type, uint8_t variant, Depth depth);

private:
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

    glm::vec2* m_position_buffer_data = nullptr;
    glm::vec2* m_position_buffer_data_ptr = nullptr;

    glm::quat* m_rotation_buffer_data = nullptr;
    glm::quat* m_rotation_buffer_data_ptr = nullptr;

    float* m_scale_buffer_data = nullptr;
    float* m_scale_buffer_data_ptr = nullptr;

    TextureAtlas m_atlas;

    uint32_t m_particle_count;
    bool m_is_metal;
};

#endif