#include "chunk.hpp"

#include <SGE/engine.hpp>
#include <SGE/renderer/macros.hpp>
#include <SGE/defines.hpp>
#include <SGE/profile.hpp>

#include <LLGL/TextureFlags.h>

#include "../types/block.hpp"
#include "../types/texture_atlas_pos.hpp"
#include "../renderer/types.hpp"

#include "../renderer/renderer.hpp"
#include "lightmap.hpp"

static constexpr float LIGHTMAP_CHUNK_WORLD_SIZE = LIGHTMAP_CHUNK_TILE_SIZE * Constants::TILE_SIZE;
static constexpr float LIGHTMAP_TO_WORLD = Constants::TILE_SIZE / Constants::SUBDIVISION;

using Constants::SUBDIVISION;
using Constants::RENDER_CHUNK_SIZE;
using Constants::RENDER_CHUNK_SIZE_U;

void RenderChunk::destroy() {
    ZoneScoped;

    const auto& context = sge::Engine::Renderer().Context();

    SGE_RESOURCE_RELEASE(m_block_instance_buffer);
    SGE_RESOURCE_RELEASE(m_wall_instance_buffer);
    SGE_RESOURCE_RELEASE(m_block_buffer_array);
    SGE_RESOURCE_RELEASE(m_wall_buffer_array);
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

static void internal_lightmap_init_area(const WorldData& world, LightMap& lightmap, const sge::IRect& area, glm::ivec2 tile_offset = {0, 0}) {
    ZoneScoped;

    const int min_y = area.min.y;
    const int max_y = area.max.y;

    const int min_x = area.min.x;
    const int max_x = area.max.x;

#ifndef SGE_DEBUG
    #pragma omp parallel for collapse(2)
#endif
    for (int y = min_y; y < max_y; ++y) {
        for (int x = min_x; x < max_x; ++x) {
            const TilePos color_pos = TilePos(x, y);
            const TilePos tile_pos = tile_offset + color_pos / SUBDIVISION;

            lightmap.set_mask(color_pos, world.solid_block_exists(tile_pos));

            std::optional<glm::vec3> light = block_light(world.get_block_type(tile_pos));
            if (light.has_value()) {
                lightmap.set_color(color_pos, light.value());
                continue;
            }

            if (tile_offset.y * SUBDIVISION + y >= world.layers.underground * SUBDIVISION) {
                lightmap.set_color(color_pos, glm::vec3(0.0f));
                continue;
            }

            if (tile_pos.x < world.playable_area.min.x || tile_pos.x > world.playable_area.max.x - 1 || world.solid_block_exists(tile_pos) || world.wall_exists(tile_pos)) {
                lightmap.set_color(color_pos, glm::vec3(0.0f));
            } else {
                lightmap.set_color(color_pos, glm::vec3(1.0f));
            }
        }
    }
}

static void blur(LightMap& lightmap, int index, glm::vec3& prev_light, float& prev_decay) {
    using Constants::LIGHT_EPSILON;

    glm::vec3 this_light = lightmap.get_color(index);

    prev_light.r = prev_light.r < LIGHT_EPSILON ? 0.0f : prev_light.r;
    prev_light.g = prev_light.g < LIGHT_EPSILON ? 0.0f : prev_light.g;
    prev_light.b = prev_light.b < LIGHT_EPSILON ? 0.0f : prev_light.b;

    if (prev_light.r < this_light.r) {
        prev_light.r = this_light.r;
    } else {
        this_light.r = prev_light.r;
    }

    if (prev_light.g < this_light.g) {
        prev_light.g = this_light.g;
    } else {
        this_light.g = prev_light.g;
    }

    if (prev_light.b < this_light.b) {
        prev_light.b = this_light.b;
    } else {
        this_light.b = prev_light.b;
    }

    lightmap.set_color(index, this_light);

    prev_light = prev_light * prev_decay;
    prev_decay = Constants::LightDecay(lightmap.get_mask(index));
}

SGE_FORCE_INLINE static void blur_line(LightMap& lightmap, int start, int end, int stride, glm::vec3& prev_light, float& prev_decay, glm::vec3& prev_light2, float& prev_decay2) {
    using Constants::LIGHT_EPSILON;

    int length = end - start;
    for (int index = 0; index < length; index += stride) {
        blur(lightmap, start + index, prev_light, prev_decay);
        blur(lightmap, end - index, prev_light2, prev_decay2);
    }
}

inline static void blur_horizontal(LightMap& lightmap, const sge::IRect& area) {
    #pragma omp parallel for
    for (int y = area.min.y; y < area.max.y; ++y) {
        glm::vec3 prev_light = lightmap.get_color({area.min.x, y});
        float prev_decay = Constants::LightDecay(lightmap.get_mask({area.min.x - 1, y}));

        glm::vec3 prev_light2 = lightmap.get_color({area.max.x - 1, y});
        float prev_decay2 = Constants::LightDecay(lightmap.get_mask({area.max.x,  y}));

        blur_line(lightmap, y * lightmap.width + area.min.x, y * lightmap.width + (area.max.x - 1), 1, prev_light, prev_decay, prev_light2, prev_decay2);
    }
}

inline static void blur_vertical(LightMap& lightmap, const sge::IRect& area) {
    #pragma omp parallel for
    for (int x = area.min.x; x < area.max.x; ++x) {
        glm::vec3 prev_light = lightmap.get_color({x, area.min.y});
        float prev_decay = Constants::LightDecay(lightmap.get_mask({x, area.min.y - 1}));

        glm::vec3 prev_light2 = lightmap.get_color({x, area.max.y - 1});
        float prev_decay2 = Constants::LightDecay(lightmap.get_mask({x, area.max.y}));

        blur_line(lightmap, area.min.y * lightmap.width + x, (area.max.y - 1) * lightmap.width + x, lightmap.width, prev_light, prev_decay, prev_light2, prev_decay2);
    }
}

static void internal_lightmap_blur_area(LightMap& lightmap, const sge::IRect& area) {
    ZoneScoped;

    blur_horizontal(lightmap, area);
    blur_vertical(lightmap, area);

    blur_horizontal(lightmap, area);
    blur_vertical(lightmap, area);

    blur_horizontal(lightmap, area);
}

constexpr int INSET = 1;

LightMap build_lightmap_chunk(glm::uvec2 index, const WorldData& world, LightMapChunkNeighbors neighbors) {
    const glm::uvec2 world_lightmap_size = world.area.size() * Constants::SUBDIVISION;
    
    glm::uvec2 chunk_size = glm::uvec2(LIGHTMAP_CHUNK_SIZE, LIGHTMAP_CHUNK_SIZE);
    if (index.x * LIGHTMAP_CHUNK_SIZE + chunk_size.x > world_lightmap_size.x) {
        chunk_size.x = world_lightmap_size.x - index.x * LIGHTMAP_CHUNK_SIZE;
    }
    if (index.y * LIGHTMAP_CHUNK_SIZE + chunk_size.y > world_lightmap_size.y) {
        chunk_size.y = world_lightmap_size.y - index.y * LIGHTMAP_CHUNK_SIZE;
    }
    
    const glm::ivec2 offset = glm::ivec2(index * LIGHTMAP_CHUNK_TILE_SIZE);

    LightMap lightmap(chunk_size + INSET * 2u);
    if (neighbors.top != nullptr) {
        SGE_ASSERT(neighbors.top->width == lightmap.width);
        memcpy(&lightmap.colors[0 * lightmap.width], &neighbors.top->colors[(neighbors.top->height - INSET - 1) * lightmap.width], (lightmap.width) * sizeof(Color));
        memcpy(&lightmap.masks[0 * lightmap.width], &neighbors.top->masks[(neighbors.top->height - INSET - 1) * lightmap.width], (lightmap.width) * sizeof(LightMask));
    }
    if (neighbors.bottom != nullptr) {
        SGE_ASSERT(neighbors.bottom->width == lightmap.width);
        memcpy(&lightmap.colors[(lightmap.height - 1) * lightmap.width], &neighbors.bottom->colors[INSET * lightmap.width], lightmap.width * sizeof(Color));
        memcpy(&lightmap.masks[(lightmap.height - 1) * lightmap.width], &neighbors.bottom->masks[INSET * lightmap.width], lightmap.width * sizeof(LightMask));
    }
    if (neighbors.left != nullptr) {
        SGE_ASSERT(neighbors.left->height == lightmap.height);
        for (int y = 0; y < lightmap.height; ++y) {
            lightmap.colors[y * lightmap.width] = neighbors.left->colors[y * neighbors.left->width + neighbors.left->width - INSET - 1];
            lightmap.masks[y * lightmap.width] = neighbors.left->masks[y * neighbors.left->width + neighbors.left->width - INSET - 1];
        }
    }
    if (neighbors.right != nullptr) {
        SGE_ASSERT(neighbors.right->height == lightmap.height);
        for (int y = 0; y < lightmap.height; ++y) {
            lightmap.colors[y * lightmap.width + lightmap.width - 1] = neighbors.right->colors[y * neighbors.right->width + INSET];
            lightmap.masks[y * lightmap.width + lightmap.width - 1] = neighbors.right->masks[y * neighbors.right->width + INSET];
        }
    }
    
    const sge::IRect lightmap_area = sge::IRect::from_top_left(glm::ivec2(0), glm::ivec2(lightmap.width, lightmap.height));
    internal_lightmap_init_area(world, lightmap, lightmap_area.inset(-1), offset - INSET / Constants::SUBDIVISION);
    internal_lightmap_blur_area(lightmap, lightmap_area);

    return lightmap;
}

StaticLightMapChunk::StaticLightMapChunk(glm::uvec2 index, LightMap t_lightmap) :
    lightmap{ std::move(t_lightmap) },
    index{ index }
{
    ZoneScoped;

    sge::Renderer& renderer = sge::Engine::Renderer();
    const auto& context = renderer.Context();

    SGE_RESOURCE_RELEASE(texture);
    SGE_RESOURCE_RELEASE(vertex_buffer);

    const glm::uvec2 chunk_size = glm::uvec2(lightmap.width, lightmap.height) - INSET * 2u;
    
    {
        LLGL::DynamicArray<Color> buffer(chunk_size.x * chunk_size.y);
        for (int y = 0; y < lightmap.height - INSET * 2; ++y) {
            memcpy(&buffer[y * chunk_size.x], &lightmap.colors[(y + INSET) * lightmap.width + INSET], chunk_size.x * sizeof(Color));
        }

        LLGL::TextureDescriptor texture_desc;
        texture_desc.extent = LLGL::Extent3D(chunk_size.x, chunk_size.y, 1);
        texture_desc.bindFlags = LLGL::BindFlags::Sampled;
        texture_desc.mipLevels = 1;

        LLGL::ImageView image_view;
        image_view.format = LLGL::ImageFormat::RGB;
        image_view.dataType = LLGL::DataType::UInt8;
        image_view.data = buffer.data();
        image_view.dataSize = chunk_size.x * chunk_size.y * sizeof(Color);

        texture = context->CreateTexture(texture_desc, &image_view);
    }

    const glm::vec2 position = glm::vec2(index) * LIGHTMAP_CHUNK_WORLD_SIZE;

    const StaticLightMapChunkVertex vertices[] = {
        StaticLightMapChunkVertex(
            position,
            glm::vec2(0.0f, 0.0f)
        ),

        StaticLightMapChunkVertex(
            position + glm::vec2(0.0f, chunk_size.y * LIGHTMAP_TO_WORLD),
            glm::vec2(0.0f, 1.0f)
        ),

        StaticLightMapChunkVertex(
            position + glm::vec2(chunk_size.x * LIGHTMAP_TO_WORLD, 0.0f),
            glm::vec2(1.0f, 0.0f)
        ),

        StaticLightMapChunkVertex(
            position + glm::vec2(chunk_size.x, chunk_size.y) * LIGHTMAP_TO_WORLD,
            glm::vec2(1.0f, 1.0f)
        )
    };

    vertex_buffer = renderer.CreateVertexBuffer(vertices, Assets::GetVertexFormat(VertexFormatAsset::StaticLightMapVertex), "StaticLightMap VertexBuffer");
}

StaticLightMapChunk::~StaticLightMapChunk() {
    const auto& context = sge::Engine::Renderer().Context();
    SGE_RESOURCE_RELEASE(texture);
    SGE_RESOURCE_RELEASE(vertex_buffer);
}