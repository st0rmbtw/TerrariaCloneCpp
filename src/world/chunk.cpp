#include "world/chunk.hpp"
#include "LLGL/ResourceFlags.h"
#include "optional.hpp"
#include "world/world.hpp"
#include "types/block.hpp"
#include "types/texture_atlas_pos.hpp"
#include "renderer/renderer.hpp"
#include "renderer/assets.hpp"

#define TILE_TYPE_BLOCK 0
#define TILE_TYPE_WALL 1

struct ChunkVertex {
    glm::vec2 position;
    glm::vec2 atlas_pos;
    uint32_t tile_id;
    uint32_t tile_type;
};

RenderChunk::RenderChunk(const glm::uvec2& index, const glm::vec2& world_pos, const World& world) :
    transform_matrix(glm::translate(glm::mat4(1.), glm::vec3(world_pos.x * RENDER_CHUNK_SIZE, world_pos.y * RENDER_CHUNK_SIZE, 0.0))),
    index(index)
{
    build_mesh(world);
}

void RenderChunk::destroy() {
    if (block_vertex_buffer) Renderer::Context()->Release(*block_vertex_buffer);
    if (wall_vertex_buffer) Renderer::Context()->Release(*wall_vertex_buffer);

    block_vertex_buffer = nullptr;
    wall_vertex_buffer = nullptr;
}

inline LLGL::BufferDescriptor GetBufferDescriptor() {
    LLGL::BufferDescriptor buffer_desc;
    buffer_desc.bindFlags = LLGL::BindFlags::VertexBuffer;
    buffer_desc.size = sizeof(ChunkVertex) * RENDER_CHUNK_SIZE_U * RENDER_CHUNK_SIZE_U;
    buffer_desc.stride = sizeof(ChunkVertex);
    buffer_desc.vertexAttribs = TilemapVertexFormat().attributes;
    return buffer_desc;
}

void RenderChunk::build_mesh(const World& world) {
    std::vector<ChunkVertex> block_vertices;
    std::vector<ChunkVertex> wall_vertices;

    if (blocks_dirty) {
        blocks_count = 0;
        block_vertices.reserve(RENDER_CHUNK_SIZE_U * RENDER_CHUNK_SIZE_U);
    }

    if (walls_dirty) {
        walls_count = 0;
        wall_vertices.reserve(RENDER_CHUNK_SIZE_U * RENDER_CHUNK_SIZE_U);
    }

    if (blocks_dirty) {
        for (uint32_t y = 0; y < RENDER_CHUNK_SIZE_U; ++y) {
            for (uint32_t x = 0; x < RENDER_CHUNK_SIZE_U; ++x) {
                const glm::uvec2 map_pos = glm::uvec2(
                    this->index.x * RENDER_CHUNK_SIZE_U + x,
                    this->index.y * RENDER_CHUNK_SIZE_U + y
                );

                const tl::optional<const Block&> block = world.get_block(map_pos);
                if (block.is_some()) {
                    blocks_count += 1;

                    const TextureAtlasPos atlas_pos = block->atlas_pos;

                    block_vertices.push_back(ChunkVertex {
                        .position = glm::vec2(x, y),
                        .atlas_pos = glm::vec2(atlas_pos.x, atlas_pos.y),
                        .tile_id = static_cast<uint32_t>(block->type),
                        .tile_type = TILE_TYPE_BLOCK
                    });
                }
            }
        }
    }

    if (walls_dirty) {
        for (uint32_t y = 0; y < RENDER_CHUNK_SIZE_U; ++y) {
            for (uint32_t x = 0; x < RENDER_CHUNK_SIZE_U; ++x) {
                const glm::uvec2 map_pos = glm::uvec2(
                    this->index.x * RENDER_CHUNK_SIZE_U + x,
                    this->index.y * RENDER_CHUNK_SIZE_U + y
                );

                const tl::optional<const Wall&> wall = world.get_wall(map_pos);
                if (wall.is_some()) {
                    walls_count += 1;

                    const TextureAtlasPos atlas_pos = wall->atlas_pos;

                    wall_vertices.push_back(ChunkVertex {
                        .position = glm::vec2(x, y),
                        .atlas_pos = glm::vec2(atlas_pos.x, atlas_pos.y),
                        .tile_id = static_cast<uint32_t>(wall->type),
                        .tile_type = TILE_TYPE_WALL
                    });
                }
            }
        }
    }

    if (!block_vertex_buffer) {
        const void* data = blocks_dirty && !blocks_empty() ? block_vertices.data() : nullptr;

        this->block_vertex_buffer = Renderer::Context()->CreateBuffer(GetBufferDescriptor(), data);
    } else if (blocks_dirty && !blocks_empty()) {
        Renderer::Context()->WriteBuffer(*block_vertex_buffer, 0, block_vertices.data(), block_vertices.size() * sizeof(ChunkVertex));
    }

    if (!wall_vertex_buffer) {
        const void* data = walls_dirty && !walls_empty() ? wall_vertices.data() : nullptr;

        this->wall_vertex_buffer = Renderer::Context()->CreateBuffer(GetBufferDescriptor(), data);
    } else if (walls_dirty && !walls_empty()) {
        Renderer::Context()->WriteBuffer(*wall_vertex_buffer, 0, wall_vertices.data(), wall_vertices.size() * sizeof(ChunkVertex));
    }

    this->blocks_dirty = false;
    this->walls_dirty = false;
}