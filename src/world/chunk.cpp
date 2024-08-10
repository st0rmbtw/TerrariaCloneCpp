#include "LLGL/ResourceFlags.h"
#include "chunk.hpp"
#include "world.hpp"
#include "../optional.hpp"
#include "../types/block.hpp"
#include "../types/texture_atlas_pos.hpp"
#include "../renderer/renderer.hpp"

#define TILE_TYPE_BLOCK 0
#define TILE_TYPE_WALL 1

struct ChunkVertex {
    glm::vec4 uv_size;
    glm::vec2 position;
    glm::vec2 world_pos;
    uint32_t tile_id;
    uint32_t tile_type;
};

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
    buffer_desc.vertexAttribs = Assets::GetVertexFormat(VertexFormatAsset::TilemapVertex).attributes;
    return buffer_desc;
}

void RenderChunk::build_mesh(const World& world) {
    const glm::vec2 tile_tex_size = glm::vec2(Assets::GetTexture(TextureAsset::Tiles).size);
    const glm::vec2 wall_tex_size = glm::vec2(Assets::GetTexture(TextureAsset::Walls).size);

    const glm::vec2 tile_size = glm::vec2(Constants::TILE_SIZE) / tile_tex_size;
    const glm::vec2 wall_size = glm::vec2(Constants::WALL_SIZE) / wall_tex_size;

    const glm::vec2 tile_padding = glm::vec2(Constants::TILE_TEXTURE_PADDING) / tile_tex_size;
    const glm::vec2 wall_padding = glm::vec2(Constants::WALL_TEXTURE_PADDING) / wall_tex_size;

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

                    const glm::vec2 atlas_pos = glm::vec2(block->atlas_pos.x, block->atlas_pos.y);

                    block_vertices.push_back(ChunkVertex {
                        .uv_size = glm::vec4(atlas_pos * (tile_size + tile_padding), tile_size),
                        .position = glm::vec2(x, y),
                        .world_pos = world_pos,
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

                    const glm::vec2 atlas_pos = glm::vec2(wall->atlas_pos.x, wall->atlas_pos.y);

                    wall_vertices.push_back(ChunkVertex {
                        .uv_size = glm::vec4(atlas_pos * (wall_size + wall_padding), wall_size),
                        .position = glm::vec2(x, y),
                        .world_pos = world_pos,
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