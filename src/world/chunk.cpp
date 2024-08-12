#include "chunk.hpp"

#include "world.hpp"
#include "../optional.hpp"
#include "../types/block.hpp"
#include "../types/texture_atlas_pos.hpp"
#include "../renderer/renderer.hpp"
#include "../renderer/types.hpp"

#define TILE_TYPE_BLOCK 0
#define TILE_TYPE_WALL 1

void RenderChunk::destroy() {
    if (block_instance_buffer) Renderer::Context()->Release(*block_instance_buffer);
    if (wall_instance_buffer) Renderer::Context()->Release(*wall_instance_buffer);

    block_instance_buffer = nullptr;
    wall_instance_buffer = nullptr;
}

inline LLGL::BufferDescriptor GetBufferDescriptor() {
    LLGL::BufferDescriptor buffer_desc;
    buffer_desc.bindFlags = LLGL::BindFlags::VertexBuffer;
    buffer_desc.size = sizeof(ChunkInstance) * RENDER_CHUNK_SIZE_U * RENDER_CHUNK_SIZE_U;
    buffer_desc.stride = sizeof(ChunkInstance);
    buffer_desc.vertexAttribs = Assets::GetVertexFormat(VertexFormatAsset::TilemapInstance).attributes;
    return buffer_desc;
}

void RenderChunk::build_mesh(const World& world) {
    std::vector<ChunkInstance> block_instance;
    std::vector<ChunkInstance> wall_instance;

    if (blocks_dirty) {
        blocks_count = 0;
        block_instance.reserve(RENDER_CHUNK_SIZE_U * RENDER_CHUNK_SIZE_U);
    }

    if (walls_dirty) {
        walls_count = 0;
        wall_instance.reserve(RENDER_CHUNK_SIZE_U * RENDER_CHUNK_SIZE_U);
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

                    block_instance.push_back(ChunkInstance {
                        .position = glm::vec2(x, y),
                        .atlas_pos = atlas_pos,
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

                    wall_instance.push_back(ChunkInstance {
                        .position = glm::vec2(x, y),
                        .atlas_pos = atlas_pos,
                        .world_pos = world_pos,
                        .tile_id = static_cast<uint32_t>(wall->type),
                        .tile_type = TILE_TYPE_WALL
                    });
                }
            }
        }
    }

    if (!block_buffer_array) {
        const void* data = blocks_dirty && !blocks_empty() ? block_instance.data() : nullptr;

        this->block_instance_buffer = Renderer::Context()->CreateBuffer(GetBufferDescriptor(), data);

        LLGL::Buffer* buffers[] = { Renderer::ChunkVertexBuffer(), block_instance_buffer };

        this->block_buffer_array = Renderer::Context()->CreateBufferArray(2, buffers);
    } else if (blocks_dirty && !blocks_empty()) {
        Renderer::Context()->WriteBuffer(*block_instance_buffer, 0, block_instance.data(), block_instance.size() * sizeof(ChunkVertex));
    }

    if (!wall_instance_buffer) {
        const void* data = walls_dirty && !walls_empty() ? wall_instance.data() : nullptr;

        this->wall_instance_buffer = Renderer::Context()->CreateBuffer(GetBufferDescriptor(), data);

        LLGL::Buffer* buffers[] = { Renderer::ChunkVertexBuffer(), wall_instance_buffer };

        this->wall_buffer_array = Renderer::Context()->CreateBufferArray(2, buffers);
    } else if (walls_dirty && !walls_empty()) {
        Renderer::Context()->WriteBuffer(*wall_instance_buffer, 0, wall_instance.data(), wall_instance.size() * sizeof(ChunkVertex));
    }

    this->blocks_dirty = false;
    this->walls_dirty = false;
}