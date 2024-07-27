#pragma once

#ifndef WORLD_CHUNK_HPP
#define WORLD_CHUNK_HPP

#include <stdint.h>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <LLGL/LLGL.h>

constexpr float RENDER_CHUNK_SIZE = 50;
constexpr uint32_t RENDER_CHUNK_SIZE_U = 50u;

struct RenderChunk {
    glm::uvec2 index;
    glm::mat4 transform_matrix;
    bool blocks_dirty;
    bool walls_dirty;
    uint16_t blocks_count;
    uint16_t walls_count;

    LLGL::Buffer* block_vertex_buffer = nullptr;
    LLGL::Buffer* wall_vertex_buffer = nullptr;

    RenderChunk(const glm::uvec2& index, const glm::vec2& world_pos, const class World& world);
    RenderChunk(RenderChunk&& other);
    ~RenderChunk();

    void build_mesh(const class World& world);

    inline bool dirty(void) const { return blocks_dirty || walls_dirty; }

    inline bool blocks_empty(void) const { return blocks_count == 0; }
    inline bool walls_empty(void) const { return walls_count == 0; }
};

#endif