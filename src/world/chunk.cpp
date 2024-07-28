#include "world/chunk.hpp"
#include "optional.hpp"
#include "world/world.hpp"
#include "types/block.hpp"
#include "types/texture_atlas_pos.hpp"
#include "renderer/renderer.hpp"
#include "renderer/assets.hpp"

#define TILE_TYPE_BLOCK 0
#define TILE_TYPE_WALL 1

struct Vertex {
    glm::vec2 position;
    glm::vec2 atlas_pos;
    uint32_t tile_id;
    uint32_t tile_type;
};

RenderChunk::RenderChunk(const glm::uvec2& index, const glm::vec2& world_pos, const World& world) :
    index(index),
    blocks_dirty(true),
    walls_dirty(true),
    blocks_count(0),
    walls_count(0),
    block_vertex_buffer(nullptr),
    wall_vertex_buffer(nullptr)
{
    const glm::mat4 translation = glm::translate(glm::mat4(1.), glm::vec3(world_pos.x * RENDER_CHUNK_SIZE, world_pos.y * RENDER_CHUNK_SIZE, 0.0));
    const glm::mat4 scale = glm::scale(glm::mat4(1.), glm::vec3(1.));
    this->transform_matrix = translation * scale;
    build_mesh(world);
}

RenderChunk::RenderChunk(RenderChunk&& other) :
    index(other.index),
    transform_matrix(other.transform_matrix),
    blocks_dirty(other.blocks_dirty),
    walls_dirty(other.walls_dirty),
    blocks_count(other.blocks_count),
    walls_count(other.walls_count),
    block_vertex_buffer(other.block_vertex_buffer),
    wall_vertex_buffer(other.wall_vertex_buffer)
{
    other.block_vertex_buffer = nullptr;    
    other.wall_vertex_buffer = nullptr;
}

RenderChunk::~RenderChunk() {
    if (block_vertex_buffer) Renderer::Context()->Release(*block_vertex_buffer);
    if (wall_vertex_buffer) Renderer::Context()->Release(*wall_vertex_buffer);
}

void RenderChunk::build_mesh(const World& world) {
    std::vector<Vertex> block_vertices;

    if (!block_vertex_buffer) {
        LLGL::BufferDescriptor buffer_desc;
        buffer_desc.bindFlags = LLGL::BindFlags::VertexBuffer;
        buffer_desc.size = sizeof(Vertex) * RENDER_CHUNK_SIZE_U * RENDER_CHUNK_SIZE_U;
        buffer_desc.stride = sizeof(Vertex);
        buffer_desc.vertexAttribs = TilemapVertexFormat().attributes;
        this->block_vertex_buffer = Renderer::Context()->CreateBuffer(buffer_desc);
    }

    if (!wall_vertex_buffer) {
        LLGL::BufferDescriptor buffer_desc;
        buffer_desc.bindFlags = LLGL::BindFlags::VertexBuffer;
        buffer_desc.size = sizeof(Vertex) * RENDER_CHUNK_SIZE_U * RENDER_CHUNK_SIZE_U;
        buffer_desc.stride = sizeof(Vertex);
        buffer_desc.vertexAttribs = TilemapVertexFormat().attributes;
        this->wall_vertex_buffer = Renderer::Context()->CreateBuffer(buffer_desc);
    }

    if (blocks_dirty) {
        blocks_count = 0;
        block_vertices.reserve(RENDER_CHUNK_SIZE_U * RENDER_CHUNK_SIZE_U);
    }

    std::vector<Vertex> wall_vertices;

    if (walls_dirty) {
        walls_count = 0;
        wall_vertices.reserve(RENDER_CHUNK_SIZE_U * RENDER_CHUNK_SIZE_U);
    }

    for (uint32_t y = 0; y < RENDER_CHUNK_SIZE_U; ++y) {
        for (uint32_t x = 0; x < RENDER_CHUNK_SIZE_U; ++x) {
            const glm::uvec2 map_pos = glm::uvec2(
                this->index.x * RENDER_CHUNK_SIZE_U + x,
                this->index.y * RENDER_CHUNK_SIZE_U + y
            );

            const tl::optional<const Block&> block = world.get_block(map_pos);
            if (blocks_dirty && block.is_some()) {
                blocks_count += 1;

                const TextureAtlasPos atlas_pos = block->atlas_pos;

                block_vertices.push_back(Vertex {
                    .position = glm::vec2(x, y),
                    .atlas_pos = glm::vec2(atlas_pos.x, atlas_pos.y),
                    .tile_id = static_cast<uint32_t>(block->type),
                    .tile_type = TILE_TYPE_BLOCK
                });
            }

            const tl::optional<const Wall&> wall = world.get_wall(map_pos);
            if (walls_dirty && wall.is_some()) {
                walls_count += 1;

                const TextureAtlasPos atlas_pos = wall->atlas_pos;

                wall_vertices.push_back(Vertex {
                    .position = glm::vec2(x, y),
                    .atlas_pos = glm::vec2(atlas_pos.x, atlas_pos.y),
                    .tile_id = static_cast<uint32_t>(wall->type),
                    .tile_type = TILE_TYPE_WALL
                });
            }
        }
    }

    if (blocks_dirty && !blocks_empty()) {
        Renderer::Context()->WriteBuffer(*block_vertex_buffer, 0, block_vertices.data(), block_vertices.size() * sizeof(Vertex));
    }

    if (walls_dirty && !walls_empty()) {
        Renderer::Context()->WriteBuffer(*wall_vertex_buffer, 0, wall_vertices.data(), wall_vertices.size() * sizeof(Vertex));
    }

    this->blocks_dirty = false;
    this->walls_dirty = false;
}