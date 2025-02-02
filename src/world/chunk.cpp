#include "chunk.hpp"

#include <tracy/Tracy.hpp>

#include "../types/block.hpp"
#include "../types/texture_atlas_pos.hpp"
#include "../renderer/types.hpp"
#include "../engine/engine.hpp"

#include "../renderer/renderer.hpp"

#include "../defines.hpp"

#define TILE_TYPE_BLOCK 0
#define TILE_TYPE_WALL 1

using Constants::SUBDIVISION;
using Constants::RENDER_CHUNK_SIZE;
using Constants::RENDER_CHUNK_SIZE_U;

void RenderChunk::destroy() {
    ZoneScopedN("WorldChunk::destroy");

    const auto& context = Engine::Renderer().Context();

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

static inline uint16_t fill_block_buffer(const WorldData& world, ChunkInstance* data, glm::uvec2 index, glm::vec2 world_pos) {
    uint16_t count = 0;

    for (uint32_t y = 0; y < RENDER_CHUNK_SIZE_U; ++y) {
        for (uint32_t x = 0; x < RENDER_CHUNK_SIZE_U; ++x) {
            const glm::uvec2 map_pos = glm::uvec2(
                index.x * RENDER_CHUNK_SIZE_U + x,
                index.y * RENDER_CHUNK_SIZE_U + y
            );

            const std::optional<Block> block = world.get_block(map_pos);
            if (block.has_value()) {
                count++;

                const glm::vec2 atlas_pos = glm::vec2(block->atlas_pos.x, block->atlas_pos.y);
                const uint16_t tile_data = pack_tile_data(static_cast<uint32_t>(block->type), TILE_TYPE_BLOCK);

                data->position = glm::vec2(x, y);
                data->atlas_pos = atlas_pos;
                data->world_pos = world_pos;
                data->tile_data = tile_data;
                data++;
            }
        }
    }

    return count;
}

static inline uint16_t fill_wall_buffer(const WorldData& world, ChunkInstance* data, glm::uvec2 index, glm::vec2 world_pos) {
    uint16_t count = 0;

    for (uint32_t y = 0; y < RENDER_CHUNK_SIZE_U; ++y) {
        for (uint32_t x = 0; x < RENDER_CHUNK_SIZE_U; ++x) {
            const glm::uvec2 map_pos = glm::uvec2(
                index.x * RENDER_CHUNK_SIZE_U + x,
                index.y * RENDER_CHUNK_SIZE_U + y
            );

            const std::optional<Wall> wall = world.get_wall(map_pos);
            if (wall.has_value()) {
                count++;

                const glm::vec2 atlas_pos = glm::vec2(wall->atlas_pos.x, wall->atlas_pos.y);
                const uint16_t tile_data = pack_tile_data(static_cast<uint32_t>(wall->type), TILE_TYPE_WALL);

                data->position = glm::vec2(x, y);
                data->atlas_pos = atlas_pos;
                data->world_pos = world_pos;
                data->tile_data = tile_data;
                data++;
            }
        }
    }

    return count;
}


void RenderChunk::build_mesh(
    const WorldData& world,
    ChunkInstance* block_data_arena,
    ChunkInstance* wall_data_arena
) {
    ZoneScopedN("WorldChunk::build_mesh");

    block_count = fill_block_buffer(world, block_data_arena, index, world_pos);
    wall_count = fill_wall_buffer(world, wall_data_arena, index, world_pos);

    const auto& context = Engine::Renderer().Context();

    {
        const size_t size = block_count * sizeof(ChunkInstance);
        const void* data = size > 0 ? block_data_arena : nullptr;

        this->block_instance_buffer = context->CreateBuffer(GetBufferDescriptor(), data);

        LLGL::Buffer* buffers[] = { GameRenderer::ChunkVertexBuffer(), block_instance_buffer };

        this->block_buffer_array = context->CreateBufferArray(2, buffers);
    }
    {
        const size_t size = wall_count * sizeof(ChunkInstance);
        const void* data = size > 0 ? wall_data_arena : nullptr;

        this->wall_instance_buffer = context->CreateBuffer(GetBufferDescriptor(), data);

        LLGL::Buffer* buffers[] = { GameRenderer::ChunkVertexBuffer(), wall_instance_buffer };

        this->wall_buffer_array = context->CreateBufferArray(2, buffers);
    }
}

void RenderChunk::rebuild_mesh(
    const WorldData& world,
    ChunkInstance* block_data_arena,
    ChunkInstance* wall_data_arena
) {
    ZoneScopedN("WorldChunk::rebuild_mesh");

    const auto& context = Engine::Renderer().Context();

    if (blocks_dirty) {
        block_count = fill_block_buffer(world, block_data_arena, index, world_pos);
        if (block_count > 0) {
            context->WriteBuffer(*block_instance_buffer, 0, block_data_arena, block_count * sizeof(ChunkInstance));
        }
    }

    if (walls_dirty) {
        wall_count = fill_wall_buffer(world, wall_data_arena, index, world_pos);
        if (wall_count > 0) {
            context->WriteBuffer(*wall_instance_buffer, 0, wall_data_arena, wall_count * sizeof(ChunkInstance));
        }
    }

    blocks_dirty = false;
    walls_dirty = false;
}