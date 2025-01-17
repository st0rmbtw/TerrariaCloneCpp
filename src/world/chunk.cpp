#include "chunk.hpp"

#include <tracy/Tracy.hpp>

#include "../types/block.hpp"
#include "../types/texture_atlas_pos.hpp"
#include "../renderer/renderer.hpp"
#include "../renderer/types.hpp"
#include "../renderer/macros.hpp"

#include "../defines.hpp"

#define TILE_TYPE_BLOCK 0
#define TILE_TYPE_WALL 1

using Constants::SUBDIVISION;
using Constants::RENDER_CHUNK_SIZE;
using Constants::RENDER_CHUNK_SIZE_U;

void RenderChunk::destroy() {
    ZoneScopedN("WorldChunk::destroy");

    RESOURCE_RELEASE(block_instance_buffer);
    RESOURCE_RELEASE(wall_instance_buffer);
}

static inline LLGL::BufferDescriptor GetBufferDescriptor() {
    LLGL::BufferDescriptor buffer_desc;
    buffer_desc.bindFlags = LLGL::BindFlags::VertexBuffer;
    buffer_desc.size = sizeof(ChunkInstance) * RENDER_CHUNK_SIZE_U * RENDER_CHUNK_SIZE_U;
    buffer_desc.stride = sizeof(ChunkInstance);
    buffer_desc.vertexAttribs = Assets::GetVertexFormat(VertexFormatAsset::TilemapInstance).attributes;
    return buffer_desc;
}

static FORCE_INLINE uint16_t pack_tile_data(uint16_t tile_id, uint8_t tile_type) {
    // 6 bits for tile_type and 10 bits for tile_id
    return (tile_type & 0x3f) | (tile_id << 6);
}

void RenderChunk::build_mesh(const WorldData& world) {
    ZoneScopedN("WorldChunk::build_mesh");

    std::vector<ChunkInstance> block_instance;
    std::vector<ChunkInstance> wall_instance;

    if (blocks_dirty) {
        blocks_count = 0;
        block_instance.reserve(RENDER_CHUNK_SIZE_U * RENDER_CHUNK_SIZE_U);

        for (uint32_t y = 0; y < RENDER_CHUNK_SIZE_U; ++y) {
            for (uint32_t x = 0; x < RENDER_CHUNK_SIZE_U; ++x) {
                const glm::uvec2 map_pos = glm::uvec2(
                    this->index.x * RENDER_CHUNK_SIZE_U + x,
                    this->index.y * RENDER_CHUNK_SIZE_U + y
                );

                const std::optional<Block> block = world.get_block(map_pos);
                if (block.has_value()) {
                    blocks_count += 1;

                    const glm::vec2 atlas_pos = glm::vec2(block->atlas_pos.x, block->atlas_pos.y);
                    const uint16_t tile_data = pack_tile_data(static_cast<uint32_t>(block->type), TILE_TYPE_BLOCK);

                    block_instance.emplace_back(
                        glm::vec2(x, y), atlas_pos, world_pos, tile_data
                    );
                }
            }
        }
    }

    if (walls_dirty) {
        walls_count = 0;
        wall_instance.reserve(RENDER_CHUNK_SIZE_U * RENDER_CHUNK_SIZE_U);

        for (uint32_t y = 0; y < RENDER_CHUNK_SIZE_U; ++y) {
            for (uint32_t x = 0; x < RENDER_CHUNK_SIZE_U; ++x) {
                const glm::uvec2 map_pos = glm::uvec2(
                    this->index.x * RENDER_CHUNK_SIZE_U + x,
                    this->index.y * RENDER_CHUNK_SIZE_U + y
                );

                const std::optional<Wall> wall = world.get_wall(map_pos);
                if (wall.has_value()) {
                    walls_count += 1;

                    const glm::vec2 atlas_pos = glm::vec2(wall->atlas_pos.x, wall->atlas_pos.y);
                    const uint16_t tile_data = pack_tile_data(static_cast<uint32_t>(wall->type), TILE_TYPE_WALL);

                    wall_instance.emplace_back(
                        glm::vec2(x, y), atlas_pos, world_pos, tile_data
                    );
                }
            }
        }
    }

    auto& context = Renderer::Context();

    if (!block_buffer_array) {
        const void* data = blocks_dirty && !blocks_empty() ? block_instance.data() : nullptr;

        this->block_instance_buffer = context->CreateBuffer(GetBufferDescriptor(), data);

        LLGL::Buffer* buffers[] = { Renderer::ChunkVertexBuffer(), block_instance_buffer };

        this->block_buffer_array = context->CreateBufferArray(2, buffers);
    } else if (blocks_dirty && !blocks_empty()) {
        context->WriteBuffer(*block_instance_buffer, 0, block_instance.data(), block_instance.size() * sizeof(ChunkInstance));
    }

    if (!wall_instance_buffer) {
        const void* data = walls_dirty && !walls_empty() ? wall_instance.data() : nullptr;

        this->wall_instance_buffer = context->CreateBuffer(GetBufferDescriptor(), data);

        LLGL::Buffer* buffers[] = { Renderer::ChunkVertexBuffer(), wall_instance_buffer };

        this->wall_buffer_array = context->CreateBufferArray(2, buffers);
    } else if (walls_dirty && !walls_empty()) {
        context->WriteBuffer(*wall_instance_buffer, 0, wall_instance.data(), wall_instance.size() * sizeof(ChunkInstance));
    }

    this->blocks_dirty = false;
    this->walls_dirty = false;
}