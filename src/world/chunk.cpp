#include "chunk.hpp"

#include <SGE/engine.hpp>
#include <SGE/renderer/macros.hpp>
#include <SGE/defines.hpp>
#include <SGE/profile.hpp>

#include "../types/block.hpp"
#include "../types/texture_atlas_pos.hpp"
#include "../renderer/types.hpp"

#include "../renderer/renderer.hpp"

using Constants::SUBDIVISION;
using Constants::RENDER_CHUNK_SIZE;
using Constants::RENDER_CHUNK_SIZE_U;

void RenderChunk::destroy() {
    ZoneScoped;

    const auto& context = sge::Engine::Renderer().Context();

    SGE_RESOURCE_RELEASE(m_block_instance_buffer);
    SGE_RESOURCE_RELEASE(m_wall_instance_buffer);
}

static inline LLGL::BufferDescriptor GetBufferDescriptor() {
    LLGL::BufferDescriptor buffer_desc;
    buffer_desc.bindFlags = LLGL::BindFlags::VertexBuffer;
    buffer_desc.size = sizeof(ChunkInstance) * RENDER_CHUNK_SIZE_U * RENDER_CHUNK_SIZE_U;
    buffer_desc.stride = sizeof(ChunkInstance);
    buffer_desc.vertexAttribs = Assets::GetVertexFormat(VertexFormatAsset::TilemapInstance).attributes;
    return buffer_desc;
}

static SGE_FORCE_INLINE uint16_t pack_tile_data(uint16_t tile_texture_id, uint8_t tile_type) {
    // 6 bits for tile_type and 10 bits for tile_texture_id
    return (tile_type & 0x3f) | (tile_texture_id << 6);
}

static SGE_FORCE_INLINE uint16_t pack_position(uint8_t x, uint8_t y) {
    return (y << 8) | x;
}

static inline uint16_t fill_block_buffer(const WorldData& world, ChunkInstance* data, glm::uvec2 index, glm::vec2 world_pos) {
    uint16_t count = 0;

    for (uint8_t y = 0; y < RENDER_CHUNK_SIZE_U; ++y) {
        for (uint8_t x = 0; x < RENDER_CHUNK_SIZE_U; ++x) {
            const glm::uvec2 map_pos = glm::uvec2(
                index.x * RENDER_CHUNK_SIZE_U + x,
                index.y * RENDER_CHUNK_SIZE_U + y
            );

            const std::optional<Block> tile = world.get_block(map_pos);
            if (tile.has_value()) {
                count++;

                const glm::vec2 atlas_pos = glm::vec2(tile->atlas_pos.x, tile->atlas_pos.y);
                const uint8_t type = tile_type(tile.value());
                const uint16_t texture_id = static_cast<uint16_t>(tile_texture_type(tile.value()));
                const uint16_t tile_data = pack_tile_data(texture_id, type);

                data->position = pack_position(x, y);
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

    for (uint8_t y = 0; y < RENDER_CHUNK_SIZE_U; ++y) {
        for (uint8_t x = 0; x < RENDER_CHUNK_SIZE_U; ++x) {
            const glm::uvec2 map_pos = glm::uvec2(
                index.x * RENDER_CHUNK_SIZE_U + x,
                index.y * RENDER_CHUNK_SIZE_U + y
            );

            const std::optional<Wall> wall = world.get_wall(map_pos);
            if (wall.has_value()) {
                count++;

                const glm::vec2 atlas_pos = glm::vec2(wall->atlas_pos.x, wall->atlas_pos.y);
                const uint16_t tile_data = pack_tile_data(static_cast<uint32_t>(wall->type), TileType::Wall);

                data->position = pack_position(x, y);
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
    ZoneScoped;

    m_block_count = fill_block_buffer(world, block_data_arena, m_index, m_world_pos);
    m_wall_count = fill_wall_buffer(world, wall_data_arena, m_index, m_world_pos);

    const auto& context = sge::Engine::Renderer().Context();

    {
        const size_t size = m_block_count * sizeof(ChunkInstance);
        const void* data = size > 0 ? block_data_arena : nullptr;

        m_block_instance_buffer = context->CreateBuffer(GetBufferDescriptor(), data);

        LLGL::Buffer* buffers[] = { GameRenderer::ChunkVertexBuffer(), m_block_instance_buffer };

        m_block_buffer_array = context->CreateBufferArray(2, buffers);
    }
    {
        const size_t size = m_wall_count * sizeof(ChunkInstance);
        const void* data = size > 0 ? wall_data_arena : nullptr;

        m_wall_instance_buffer = context->CreateBuffer(GetBufferDescriptor(), data);

        LLGL::Buffer* buffers[] = { GameRenderer::ChunkVertexBuffer(), m_wall_instance_buffer };

        m_wall_buffer_array = context->CreateBufferArray(2, buffers);
    }
}

void RenderChunk::rebuild_mesh(
    const WorldData& world,
    ChunkInstance* block_data_arena,
    ChunkInstance* wall_data_arena
) {
    ZoneScoped;

    const auto& context = sge::Engine::Renderer().Context();

    if (m_blocks_dirty) {
        m_block_count = fill_block_buffer(world, block_data_arena, m_index, m_world_pos);
        if (m_block_count > 0) {
            context->WriteBuffer(*m_block_instance_buffer, 0, block_data_arena, m_block_count * sizeof(ChunkInstance));
        }
    }

    if (m_walls_dirty) {
        m_wall_count = fill_wall_buffer(world, wall_data_arena, m_index, m_world_pos);
        if (m_wall_count > 0) {
            context->WriteBuffer(*m_wall_instance_buffer, 0, wall_data_arena, m_wall_count * sizeof(ChunkInstance));
        }
    }

    m_blocks_dirty = false;
    m_walls_dirty = false;
}