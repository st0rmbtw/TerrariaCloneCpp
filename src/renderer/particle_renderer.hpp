
#ifndef TERRARIA_RENDERER_PARTICLE_RENDERER
#define TERRARIA_RENDERER_PARTICLE_RENDERER

#pragma once

#include <LLGL/Buffer.h>
#include <LLGL/PipelineState.h>

#include "../particles.hpp"
#include "../types/texture_atlas.hpp"

struct ParticleVertex {
    glm::vec2 position;
    glm::quat rotation;
    glm::vec2 uv;
    glm::vec2 tex_size;
    float scale;
};

class ParticleRenderer {
public:
    void init();
    void render();
    void terminate();

    void draw_particle(const glm::vec2& position, const glm::quat& rotation, float scale, Particle::Type type, uint8_t variant);

private:
    LLGL::PipelineState* m_pipeline = nullptr;
    LLGL::Buffer* m_vertex_buffer = nullptr;
    ParticleVertex* m_buffer = nullptr;
    ParticleVertex* m_buffer_ptr = nullptr;
    TextureAtlas m_atlas;

    size_t m_particle_count;
};

#endif