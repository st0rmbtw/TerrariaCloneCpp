#ifndef WORLD_CHUNK_HPP
#define WORLD_CHUNK_HPP

#pragma once

#include <stdint.h>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <LLGL/LLGL.h>

constexpr float RENDER_CHUNK_SIZE = 75;
constexpr uint32_t RENDER_CHUNK_SIZE_U = 75u;

struct RenderChunk {
    glm::mat4 transform_matrix;
    glm::uvec2 index;
    LLGL::Buffer* block_vertex_buffer = nullptr;
    LLGL::Buffer* wall_vertex_buffer = nullptr;
    uint16_t blocks_count = 0;
    uint16_t walls_count = 0;
    bool blocks_dirty = true;
    bool walls_dirty = true;

    RenderChunk(const glm::uvec2& index, const glm::vec2& world_pos, const class World& world);
    RenderChunk(RenderChunk&& other) noexcept;
    ~RenderChunk();

    void build_mesh(const class World& world);

    [[nodiscard]] inline bool dirty() const { return blocks_dirty || walls_dirty; }

    [[nodiscard]] inline bool blocks_empty() const { return blocks_count == 0; }
    [[nodiscard]] inline bool walls_empty() const { return walls_count == 0; }
};

#endif