#include "world_renderer.hpp"

#include <LLGL/Utils/Utility.h>
#include <LLGL/PipelineStateFlags.h>
#include <LLGL/ResourceHeapFlags.h>
#include <LLGL/TextureFlags.h>
#include <LLGL/Types.h>
#include <LLGL/PipelineLayoutFlags.h>
#include <LLGL/ResourceFlags.h>
#include <LLGL/ShaderFlags.h>

#include <cstddef>
#include <glm/gtc/type_ptr.hpp>

#include <tracy/Tracy.hpp>

#include "../assets.hpp"
#include "../world/chunk.hpp"
#include "../utils.hpp"
#include "../world/utils.hpp"
#include "../engine/engine.hpp"
#include "../engine/renderer/macros.hpp"

#include "types.hpp"

static constexpr uint32_t LIGHTMAP_CHUNK_TILE_SIZE = 500;
static constexpr uint32_t LIGHTMAP_CHUNK_SIZE = LIGHTMAP_CHUNK_TILE_SIZE * Constants::SUBDIVISION;
static constexpr float LIGHTMAP_CHUNK_WORLD_SIZE = LIGHTMAP_CHUNK_TILE_SIZE * Constants::TILE_SIZE;
static constexpr float LIGHTMAP_TO_WORLD = Constants::TILE_SIZE / Constants::SUBDIVISION;

static constexpr float TILE_DEPTH = 0.3f;
static constexpr float WALL_DEPTH = 0.1f;

struct ALIGN(16) TileTextureData {
    glm::vec2 tex_size;
    glm::vec2 tex_padding;
    glm::vec2 size;
    glm::vec2 offset;
    float depth;
};

void WorldRenderer::init(const LLGL::RenderPass* static_lightmap_render_pass) {
    ZoneScopedN("WorldRenderer::init");

    using Constants::TILE_SIZE;
    using Constants::WALL_SIZE;
    using Constants::TORCH_SIZE;

    m_renderer = &Engine::Renderer();

    const auto& context = m_renderer->Context();
    const RenderBackend backend = m_renderer->Backend();

    {
        const glm::vec2 tile_tex_size = glm::vec2(Assets::GetTexture(TextureAsset::Tiles).size());
        const glm::vec2 tile_padding = glm::vec2(Constants::TILE_TEXTURE_PADDING) / tile_tex_size;

        const glm::vec2 wall_tex_size = glm::vec2(Assets::GetTexture(TextureAsset::Walls).size());
        const glm::vec2 wall_padding = glm::vec2(Constants::WALL_TEXTURE_PADDING) / wall_tex_size;

        const TileTextureData texture_data[] = {
            TileTextureData{tile_tex_size, tile_padding, glm::vec2(TILE_SIZE),  glm::vec2(0.0f), TILE_DEPTH}, // Tile
            TileTextureData{wall_tex_size, wall_padding, glm::vec2(WALL_SIZE),  glm::vec2(-TILE_SIZE * 0.5f), WALL_DEPTH}, // Wall
            TileTextureData{tile_tex_size, tile_padding, glm::vec2(TORCH_SIZE), glm::vec2(-2.0f, 0.0f), TILE_DEPTH}, // Torch
        };
    
        LLGL::BufferDescriptor desc;
        desc.size = sizeof(texture_data);
        desc.bindFlags = LLGL::BindFlags::ConstantBuffer;

        m_tile_texture_data_buffer = context->CreateBuffer(desc, texture_data);
    }

    {
        LLGL::PipelineLayoutDescriptor pipelineLayoutDesc;
        pipelineLayoutDesc.heapBindings = {
            LLGL::BindingDescriptor(
                "GlobalUniformBuffer",
                LLGL::ResourceType::Buffer,
                LLGL::BindFlags::ConstantBuffer,
                LLGL::StageFlags::VertexStage,
                LLGL::BindingSlot(2)
            ),
            LLGL::BindingDescriptor(
                "TileTextureDataBuffer",
                LLGL::ResourceType::Buffer,
                LLGL::BindFlags::ConstantBuffer,
                LLGL::StageFlags::VertexStage,
                LLGL::BindingSlot(3)
            ),
        };
        pipelineLayoutDesc.bindings = {
            LLGL::BindingDescriptor("u_texture_array", LLGL::ResourceType::Texture, LLGL::BindFlags::Sampled, LLGL::StageFlags::FragmentStage, LLGL::BindingSlot(4)),
        };
        pipelineLayoutDesc.staticSamplers = {
            LLGL::StaticSamplerDescriptor("u_sampler", LLGL::StageFlags::FragmentStage, 5, Assets::GetSampler(TextureSampler::Nearest).descriptor()),
        };
        pipelineLayoutDesc.combinedTextureSamplers = {
            LLGL::CombinedTextureSamplerDescriptor{ "u_texture_array", "u_texture_array", "u_sampler", 4 }
        };

        LLGL::PipelineLayout* pipelineLayout = context->CreatePipelineLayout(pipelineLayoutDesc);

        const LLGL::ResourceViewDescriptor resource_views[] = {
            m_renderer->GlobalUniformBuffer(), m_tile_texture_data_buffer
        };

        m_resource_heap = context->CreateResourceHeap(pipelineLayout, resource_views);

        const ShaderPipeline& tilemap_shader = Assets::GetShader(ShaderAsset::TilemapShader);

        LLGL::GraphicsPipelineDescriptor pipelineDesc;
        pipelineDesc.debugName = "World Pipeline";
        pipelineDesc.vertexShader = tilemap_shader.vs;
        pipelineDesc.fragmentShader = tilemap_shader.ps;
        pipelineDesc.geometryShader = tilemap_shader.gs;
        pipelineDesc.pipelineLayout = pipelineLayout;
        pipelineDesc.indexFormat = LLGL::Format::R16UInt;
        pipelineDesc.primitiveTopology = LLGL::PrimitiveTopology::TriangleStrip;
        pipelineDesc.rasterizer.frontCCW = true;
        pipelineDesc.depth = LLGL::DepthDescriptor {
            .testEnabled = true,
            .writeEnabled = true,
            .compareOp = LLGL::CompareOp::GreaterEqual
        };
        pipelineDesc.blend = LLGL::BlendDescriptor {
            .targets = {
                LLGL::BlendTargetDescriptor {
                    .blendEnabled = true,
                    .srcColor = LLGL::BlendOp::SrcAlpha,
                    .dstColor = LLGL::BlendOp::InvSrcAlpha,
                    .srcAlpha = LLGL::BlendOp::SrcAlpha,
                    .dstAlpha = LLGL::BlendOp::InvSrcAlpha
                }
            }
        };

        m_pipeline = context->CreatePipelineState(pipelineDesc);
    }

    // =======================================================

    LLGL::BufferDescriptor light_buffer;
    light_buffer.bindFlags = LLGL::BindFlags::Sampled;
    light_buffer.stride = sizeof(Light);
    light_buffer.size = sizeof(Light) * WORLD_MAX_LIGHT_COUNT;
    m_light_buffer = context->CreateBuffer(light_buffer);

    {
        LLGL::PipelineLayoutDescriptor lightInitPipelineLayoutDesc;
        lightInitPipelineLayoutDesc.heapBindings = {
            LLGL::BindingDescriptor(
                "GlobalUniformBuffer",
                LLGL::ResourceType::Buffer,
                LLGL::BindFlags::ConstantBuffer,
                LLGL::StageFlags::ComputeStage,
                LLGL::BindingSlot(3)
            ),
            LLGL::BindingDescriptor(
                "LightBuffer",
                LLGL::ResourceType::Buffer,
                LLGL::BindFlags::Sampled,
                LLGL::StageFlags::ComputeStage,
                LLGL::BindingSlot(4)
            ),
            LLGL::BindingDescriptor(
                "LightTexture",
                LLGL::ResourceType::Texture,
                LLGL::BindFlags::Storage,
                LLGL::StageFlags::ComputeStage,
                LLGL::BindingSlot(6)
            ),
        };

        LLGL::PipelineLayout* lightInitPipelineLayout = context->CreatePipelineLayout(lightInitPipelineLayoutDesc);

        const LLGL::ResourceViewDescriptor lightInitResourceViews[] = {
            m_renderer->GlobalUniformBuffer(), m_light_buffer, nullptr
        };

        LLGL::ResourceHeapDescriptor lightResourceHeapDesc;
        lightResourceHeapDesc.pipelineLayout = lightInitPipelineLayout;
        lightResourceHeapDesc.numResourceViews = ARRAY_LEN(lightInitResourceViews);

        m_light_init_resource_heap = context->CreateResourceHeap(lightResourceHeapDesc, lightInitResourceViews);

        LLGL::ComputePipelineDescriptor lightPipelineDesc;
        lightPipelineDesc.debugName = "WorldLightSetLightSourcesComputePipeline";
        lightPipelineDesc.pipelineLayout = lightInitPipelineLayout;
        lightPipelineDesc.computeShader = Assets::GetComputeShader(ComputeShaderAsset::LightSetLightSources);

        m_light_set_light_sources_pipeline = context->CreatePipelineState(lightPipelineDesc);
    }
    {
        LLGL::PipelineLayoutDescriptor lightBlurPipelineLayoutDesc;
        lightBlurPipelineLayoutDesc.heapBindings = {
            LLGL::BindingDescriptor(
                "GlobalUniformBuffer",
                LLGL::ResourceType::Buffer,
                LLGL::BindFlags::ConstantBuffer,
                LLGL::StageFlags::ComputeStage,
                LLGL::BindingSlot(3)
            ),
            LLGL::BindingDescriptor(
                "TileTexture",
                LLGL::ResourceType::Texture,
                backend.IsOpenGL() ? LLGL::BindFlags::Storage : LLGL::BindFlags::Sampled,
                LLGL::StageFlags::ComputeStage,
                LLGL::BindingSlot(5)
            ),
            LLGL::BindingDescriptor(
                "LightTexture",
                LLGL::ResourceType::Texture,
                LLGL::BindFlags::Storage,
                LLGL::StageFlags::ComputeStage,
                LLGL::BindingSlot(6)
            ),
        };
        lightBlurPipelineLayoutDesc.uniforms = {
            LLGL::UniformDescriptor("uniform_min", LLGL::UniformType::UInt2),
            LLGL::UniformDescriptor("uniform_max", LLGL::UniformType::UInt2),
        };

        LLGL::PipelineLayout* lightBlurPipelineLayout = context->CreatePipelineLayout(lightBlurPipelineLayoutDesc);

        const LLGL::ResourceViewDescriptor lightBlurResourceViews[] = {
            m_renderer->GlobalUniformBuffer(), nullptr, nullptr
        };

        LLGL::ResourceHeapDescriptor lightBlurResourceHeapDesc;
        lightBlurResourceHeapDesc.pipelineLayout = lightBlurPipelineLayout;
        lightBlurResourceHeapDesc.numResourceViews = ARRAY_LEN(lightBlurResourceViews);

        m_light_blur_resource_heap = context->CreateResourceHeap(lightBlurResourceHeapDesc, lightBlurResourceViews);

        LLGL::ComputePipelineDescriptor lightPipelineDesc;
        lightPipelineDesc.pipelineLayout = lightBlurPipelineLayout;

        lightPipelineDesc.debugName = "WorldLightVerticalComputePipeline";
        lightPipelineDesc.computeShader = Assets::GetComputeShader(ComputeShaderAsset::LightVertical);
        m_light_vertical_pipeline = context->CreatePipelineState(lightPipelineDesc);

        lightPipelineDesc.debugName = "WorldLightHorizontalComputePipeline";
        lightPipelineDesc.computeShader = Assets::GetComputeShader(ComputeShaderAsset::LightHorizontal);
        m_light_horizontal_pipeline = context->CreatePipelineState(lightPipelineDesc);
    }
    {
        LLGL::PipelineLayoutDescriptor lightmapPipelineLayoutDesc;
        lightmapPipelineLayoutDesc.heapBindings = {
            LLGL::BindingDescriptor(
                "GlobalUniformBuffer",
                LLGL::ResourceType::Buffer,
                LLGL::BindFlags::ConstantBuffer,
                LLGL::StageFlags::VertexStage,
                LLGL::BindingSlot(2)
            ),
        };
        lightmapPipelineLayoutDesc.bindings = {
            LLGL::BindingDescriptor(
                "u_texture",
                LLGL::ResourceType::Texture,
                LLGL::BindFlags::Sampled,
                LLGL::StageFlags::FragmentStage,
                LLGL::BindingSlot(3)
            ),
        };
        lightmapPipelineLayoutDesc.staticSamplers = {
            LLGL::StaticSamplerDescriptor("u_sampler", LLGL::StageFlags::FragmentStage, LLGL::BindingSlot(4), Assets::GetSampler(TextureSampler::Nearest).descriptor())
        };
        lightmapPipelineLayoutDesc.combinedTextureSamplers = {
            LLGL::CombinedTextureSamplerDescriptor{ "u_texture", "u_texture", "u_sampler", 3 }
        };

        LLGL::PipelineLayout* lightmapPipelineLayout = context->CreatePipelineLayout(lightmapPipelineLayoutDesc);

        const LLGL::ResourceViewDescriptor lightmapResourceViews[] = {
            m_renderer->GlobalUniformBuffer()
        };

        m_lightmap_resource_heap = context->CreateResourceHeap(lightmapPipelineLayout, lightmapResourceViews);

        const ShaderPipeline lightmap_shader = Assets::GetShader(ShaderAsset::StaticLightMapShader);

        LLGL::GraphicsPipelineDescriptor lightPipelineDesc;
        lightPipelineDesc.debugName = "WorldStaticLightMapPipeline";
        lightPipelineDesc.vertexShader = lightmap_shader.vs;
        lightPipelineDesc.fragmentShader = lightmap_shader.ps;
        lightPipelineDesc.pipelineLayout = lightmapPipelineLayout;
        lightPipelineDesc.indexFormat = LLGL::Format::R16UInt;
        lightPipelineDesc.primitiveTopology = LLGL::PrimitiveTopology::TriangleStrip;
        lightPipelineDesc.rasterizer.frontCCW = true;
        lightPipelineDesc.renderPass = static_lightmap_render_pass;
        lightPipelineDesc.blend = LLGL::BlendDescriptor {
            .targets = {
                LLGL::BlendTargetDescriptor {
                    .blendEnabled = false,
                }
            }
        };

        m_lightmap_pipeline = context->CreatePipelineState(lightPipelineDesc);
    }
}

void WorldRenderer::init_textures(const WorldData& world) {
    ZoneScopedN("WorldRenderer::init_textures");

    using Constants::SUBDIVISION;

    auto& context = m_renderer->Context();
    const RenderBackend backend = m_renderer->Backend();

    RESOURCE_RELEASE(m_light_texture);
    RESOURCE_RELEASE(m_tile_texture);
    RESOURCE_RELEASE(m_light_texture_target);

    {
        LLGL::TextureDescriptor light_texture_desc;
        light_texture_desc.type      = LLGL::TextureType::Texture2D;
        light_texture_desc.format    = LLGL::Format::RGBA8UNorm;
        light_texture_desc.extent    = LLGL::Extent3D(world.lightmap.width, world.lightmap.height, 1);
        light_texture_desc.miscFlags = 0;
        light_texture_desc.bindFlags = LLGL::BindFlags::Storage | LLGL::BindFlags::Sampled | LLGL::BindFlags::ColorAttachment;
        light_texture_desc.mipLevels = 1;

        const size_t size = world.lightmap.width * world.lightmap.height * 3;
        uint8_t* data = new uint8_t[size];
        memset(data, 0, size);

        LLGL::ImageView image_view;
        image_view.format   = LLGL::ImageFormat::RGB;
        image_view.dataType = LLGL::DataType::UInt8;
        image_view.data     = data;
        image_view.dataSize = size;

        m_light_texture = context->CreateTexture(light_texture_desc, &image_view);
    }

    LLGL::TextureDescriptor tile_texture_desc;
    tile_texture_desc.type      = LLGL::TextureType::Texture2D;
    tile_texture_desc.format    = LLGL::Format::R8UInt;
    tile_texture_desc.extent    = LLGL::Extent3D(world.area.width(), world.area.height(), 1);
    tile_texture_desc.miscFlags = 0;
    tile_texture_desc.bindFlags = backend.IsOpenGL() ? LLGL::BindFlags::Storage : LLGL::BindFlags::Sampled;
    tile_texture_desc.mipLevels = 1;

    {
        const size_t size = world.area.width() * world.area.height();
        uint8_t* data = new uint8_t[size];
        for (int y = 0; y < world.area.height(); ++y) {
            for (int x = 0; x < world.area.width(); ++x) {
                uint8_t tile = world.tile_exists(TilePos(x, y)) ? 1 : 0;
                data[y * world.area.width() + x] = tile;
            }
        }

        LLGL::ImageView image_view;
        image_view.format   = LLGL::ImageFormat::R;
        image_view.dataType = LLGL::DataType::UInt8;
        image_view.data     = data;
        image_view.dataSize = size;

        m_tile_texture = context->CreateTexture(tile_texture_desc, &image_view);
    }

    context->WriteResourceHeap(*m_light_init_resource_heap, 2, {m_light_texture});
    context->WriteResourceHeap(*m_light_blur_resource_heap, 1, {m_tile_texture, m_light_texture});

    LLGL::RenderTargetDescriptor lightTextureRenderTarget;
    lightTextureRenderTarget.resolution = LLGL::Extent2D(world.lightmap.width, world.lightmap.height);
    lightTextureRenderTarget.colorAttachments[0].texture = m_light_texture;

    m_light_texture_target = context->CreateRenderTarget(lightTextureRenderTarget);
}

void WorldRenderer::init_lightmap_chunks(const WorldData& world) {
    using Constants::TILE_SIZE;

    const auto& context = m_renderer->Context();

    const LightMap& lightmap = world.lightmap;

    m_lightmap_width = lightmap.width;
    m_lightmap_height = lightmap.height;

    const uint32_t cols = (lightmap.width + LIGHTMAP_CHUNK_SIZE - 1u) / LIGHTMAP_CHUNK_SIZE;
    const uint32_t rows = (lightmap.height + LIGHTMAP_CHUNK_SIZE - 1u) / LIGHTMAP_CHUNK_SIZE;

    for (uint32_t j = 0; j < rows; ++j) {
        for (uint32_t i = 0; i < cols; ++i) {
            LLGL::TextureDescriptor texture_desc;
            
            glm::uvec2 chunk_size = glm::uvec2(LIGHTMAP_CHUNK_SIZE, LIGHTMAP_CHUNK_SIZE);

            if (chunk_size.x > m_lightmap_width - i * LIGHTMAP_CHUNK_SIZE) {
                chunk_size.x = m_lightmap_width - i * LIGHTMAP_CHUNK_SIZE;
            }

            if (chunk_size.y > m_lightmap_height - j * LIGHTMAP_CHUNK_SIZE) {
                chunk_size.y = m_lightmap_height - j * LIGHTMAP_CHUNK_SIZE;
            }

            texture_desc.extent = LLGL::Extent3D(chunk_size.x, chunk_size.y, 1);
            texture_desc.bindFlags = LLGL::BindFlags::Sampled;
            texture_desc.mipLevels = 1;
            texture_desc.miscFlags = LLGL::MiscFlags::DynamicUsage;

            Color* buffer = checked_alloc<Color>(chunk_size.x * chunk_size.y);
            for (uint32_t y = 0; y < chunk_size.y; ++y) {
                memcpy(&buffer[y * chunk_size.x], &lightmap.colors[(j * LIGHTMAP_CHUNK_SIZE + y) * m_lightmap_width + i * LIGHTMAP_CHUNK_SIZE], chunk_size.x * sizeof(Color));
            }

            LLGL::ImageView image_view;
            image_view.format = LLGL::ImageFormat::RGBA;
            image_view.dataType = LLGL::DataType::UInt8;
            image_view.data = buffer;
            image_view.dataSize = chunk_size.x * chunk_size.y * sizeof(Color);

            LLGL::Texture* texture = context->CreateTexture(texture_desc, &image_view);
            free(buffer);

            const StaticLightMapChunkVertex vertices[] = {
                StaticLightMapChunkVertex(
                    glm::vec2(i, j) * LIGHTMAP_CHUNK_WORLD_SIZE,
                    glm::vec2(0.0f, 0.0f)
                ),

                StaticLightMapChunkVertex(
                    glm::vec2(i, j) * LIGHTMAP_CHUNK_WORLD_SIZE + glm::vec2(0.0f, chunk_size.y * LIGHTMAP_TO_WORLD),
                    glm::vec2(0.0f, 1.0f)
                ),

                StaticLightMapChunkVertex(
                    glm::vec2(i, j) * LIGHTMAP_CHUNK_WORLD_SIZE + glm::vec2(chunk_size.x * LIGHTMAP_TO_WORLD, 0.0f),
                    glm::vec2(1.0f, 0.0f)
                ),
                
                StaticLightMapChunkVertex(
                    glm::vec2(i, j) * LIGHTMAP_CHUNK_WORLD_SIZE + glm::vec2(chunk_size) * LIGHTMAP_TO_WORLD,
                    glm::vec2(1.0f, 1.0f)
                )
            };

            m_lightmap_chunks[glm::uvec2(i, j)] = LightMapChunk {
                .texture = texture,
                .vertex_buffer = m_renderer->CreateVertexBuffer(vertices, Assets::GetVertexFormat(VertexFormatAsset::StaticLightMapVertex), "StaticLightMap VertexBuffer")
            };
        }
    }
}

FORCE_INLINE static void internal_update_world_lightmap(const WorldData& world, const LightMapTaskResult& result) {
    for (int y = 0; y < result.height; ++y) {
        memcpy(&world.lightmap.colors[(result.offset_y + y) * world.lightmap.width + result.offset_x], &result.data[y * result.width], result.width * sizeof(Color));
        memcpy(&world.lightmap.masks[(result.offset_y + y) * world.lightmap.width + result.offset_x], &result.mask[y * result.width], result.width * sizeof(LightMask));
    }
}

void WorldRenderer::update_lightmap_texture(WorldData& world) {
    ZoneScopedN("WorldRenderer::update_lightmap_texture");

    const auto& context = m_renderer->Context();

    LLGL::ImageView image_view;
    image_view.format   = LLGL::ImageFormat::RGBA;
    image_view.dataType = LLGL::DataType::UInt8;

    for (auto it = world.lightmap_tasks.cbegin(); it != world.lightmap_tasks.cend();) {
        const LightMapTask& task = *it;
        const LightMapTaskResult result = task.result->load();

        if (result.is_complete) {
            ZoneScopedN("WorldRenderer::process_lightmap_task_result");

            internal_update_world_lightmap(world, result);

            glm::uvec2 offset = glm::uvec2(result.offset_x, result.offset_y);
            const glm::uvec2 size = glm::uvec2(result.width, result.height);

            const glm::uvec2 start_chunk_pos = offset / LIGHTMAP_CHUNK_SIZE;

            glm::uvec2 remaining_size = size;
            glm::uvec2 chunk_pos = start_chunk_pos;
            glm::uvec2 write_offset = glm::uvec2(0);

            while (remaining_size.y > 0) {
                const uint32_t write_height = glm::min(offset.y + remaining_size.y, chunk_pos.y * LIGHTMAP_CHUNK_SIZE + LIGHTMAP_CHUNK_SIZE) - offset.y;

                while (remaining_size.x > 0) {
                    const uint32_t write_width = glm::min(offset.x + remaining_size.x, chunk_pos.x * LIGHTMAP_CHUNK_SIZE + LIGHTMAP_CHUNK_SIZE) - offset.x;

                    const LightMapChunk& lightmap_chunk = m_lightmap_chunks.find(chunk_pos)->second;

                    const glm::uvec2 texture_offset = offset % LIGHTMAP_CHUNK_SIZE;

                    image_view.data     = &result.data[write_offset.y * result.width + write_offset.x];
                    image_view.dataSize = write_width * write_height * sizeof(Color);
                    image_view.rowStride = result.width * sizeof(Color);
                    context->WriteTexture(*lightmap_chunk.texture, LLGL::TextureRegion(LLGL::Offset3D(texture_offset.x, texture_offset.y, 0), LLGL::Extent3D(write_width, write_height, 1)), image_view);

                    offset.x = 0;
                    remaining_size.x -= write_width;
                    write_offset.x += write_width;
                    chunk_pos.x += 1;
                }

                remaining_size.y -= write_height;
                write_offset.y += write_height;
                remaining_size.x = size.x;
                write_offset.x = 0;
                offset.x = result.offset_x;
                offset.y = 0;
                chunk_pos.x = start_chunk_pos.x;
                chunk_pos.y += 1;
            }

            delete[] result.data;
            delete[] result.mask;
            it = world.lightmap_tasks.erase(it);
            continue;
        }

        ++it;
    }

}

void WorldRenderer::update_tile_texture(WorldData& world) {
    LLGL::ImageView image_view;
    image_view.format   = LLGL::ImageFormat::R;
    image_view.dataType = LLGL::DataType::UInt8;
    image_view.dataSize = 1;

    while (!world.changed_tiles.empty()) {
        auto [pos, value] = world.changed_tiles.back();
        world.changed_tiles.pop_back();

        image_view.data     = &value;
        m_renderer->Context()->WriteTexture(*m_tile_texture, LLGL::TextureRegion(LLGL::Offset3D(pos.x, pos.y, 0), LLGL::Extent3D(1, 1, 1)), image_view);
    }
}

void WorldRenderer::render(const ChunkManager& chunk_manager) {
    ZoneScopedN("WorldRenderer::render");

    auto* const commands = m_renderer->CommandBuffer();

    commands->SetPipelineState(*m_pipeline);

    const Texture& walls_texture = Assets::GetTexture(TextureAsset::Walls);
    const Texture& tiles_texture = Assets::GetTexture(TextureAsset::Tiles);

    for (const glm::uvec2& pos : chunk_manager.visible_chunks()) {
        const RenderChunk& chunk = chunk_manager.render_chunks().find(pos)->second;

        if (chunk.wall_count > 0) {
            commands->SetVertexBufferArray(*chunk.wall_buffer_array);
            commands->SetResource(0, walls_texture);
            commands->SetResourceHeap(*m_resource_heap);

            commands->DrawInstanced(4, 0, chunk.wall_count);
        }

        if (chunk.block_count > 0) {
            commands->SetVertexBufferArray(*chunk.block_buffer_array);
            commands->SetResource(0, tiles_texture);
            commands->SetResourceHeap(*m_resource_heap);

            commands->DrawInstanced(4, 0, chunk.block_count);
        }
    }
}

static math::URect get_chunk_range(const math::Rect& camera_fov, glm::uvec2 lightmap_size) {
    using Constants::SUBDIVISION;
    using Constants::TILE_SIZE;

    uint32_t left = 0;
    uint32_t right = 0;
    uint32_t bottom = 0;
    uint32_t top = 0;

    if (camera_fov.min.x > 0.0f) {
        left = (camera_fov.min.x * (SUBDIVISION / TILE_SIZE)) / LIGHTMAP_CHUNK_SIZE;
    }
    if (camera_fov.max.x > 0.0f) {
        right = (camera_fov.max.x * (SUBDIVISION / TILE_SIZE) + LIGHTMAP_CHUNK_SIZE - 1) / LIGHTMAP_CHUNK_SIZE;
    }
    if (camera_fov.min.y > 0.0f) {
        top = (camera_fov.min.y * (SUBDIVISION / TILE_SIZE)) / LIGHTMAP_CHUNK_SIZE;
    }
    if (camera_fov.max.y > 0.0f) {
        bottom = (camera_fov.max.y * (SUBDIVISION / TILE_SIZE) + LIGHTMAP_CHUNK_SIZE - 1) / LIGHTMAP_CHUNK_SIZE;
    }

    const glm::uvec2 chunk_max_pos = (lightmap_size + LIGHTMAP_CHUNK_SIZE - 1u) / LIGHTMAP_CHUNK_SIZE;

    if (right > chunk_max_pos.x) right = chunk_max_pos.x;
    if (bottom > chunk_max_pos.y) bottom = chunk_max_pos.y;

    return {glm::uvec2(left, top), glm::uvec2(right, bottom)};
}

void WorldRenderer::render_lightmap(const Camera& camera) {
    const math::Rect camera_fov = utils::get_camera_fov(camera);
    const math::URect chunk_range = get_chunk_range(camera_fov, glm::uvec2(m_lightmap_width, m_lightmap_height));

    auto* const commands = m_renderer->CommandBuffer();

    commands->SetPipelineState(*m_lightmap_pipeline);
    commands->SetResourceHeap(*m_lightmap_resource_heap);

    for (uint32_t y = chunk_range.min.y; y < chunk_range.max.y; ++y) {
        for (uint32_t x = chunk_range.min.x; x < chunk_range.max.x; ++x) {
            const glm::uvec2 chunk_pos = glm::uvec2(x, y);

            const LightMapChunk& lightmap_chunk = m_lightmap_chunks.find(chunk_pos)->second;

            commands->SetVertexBuffer(*lightmap_chunk.vertex_buffer);
            commands->SetResource(0, *lightmap_chunk.texture);
            commands->Draw(4, 0);
        }
    }
}

void WorldRenderer::compute_light(const Camera& camera, const World& world) {
    ZoneScopedN("WorldRenderer::compute_light");

    if (world.light_count() == 0) return;

    auto* const commands = m_renderer->CommandBuffer();

    const size_t size = world.light_count() * sizeof(Light);
    commands->UpdateBuffer(*m_light_buffer, 0, world.lights(), size);

    constexpr uint32_t WORKGROUP = 16;

    const glm::ivec2 proj_area_min = glm::ivec2((camera.position() + camera.get_projection_area().min) / Constants::TILE_SIZE) - static_cast<int>(WORKGROUP);
    const glm::ivec2 proj_area_max = glm::ivec2((camera.position() + camera.get_projection_area().max) / Constants::TILE_SIZE) + static_cast<int>(WORKGROUP);

    const math::URect blur_area = math::URect(
        glm::uvec2(glm::max(proj_area_min * Constants::SUBDIVISION, glm::ivec2(0))),
        glm::uvec2(glm::max(proj_area_max * Constants::SUBDIVISION, glm::ivec2(0)))
    );

    const uint32_t grid_w = blur_area.width() / WORKGROUP;
    const uint32_t grid_h = blur_area.height() / WORKGROUP;

    if (grid_w * grid_h == 0) return;

    commands->PushDebugGroup("CS Light SetLightSources");
    {
        commands->SetPipelineState(*m_light_set_light_sources_pipeline);
        commands->SetResourceHeap(*m_light_init_resource_heap);
        commands->Dispatch(world.light_count(), 1, 1);
    }
    commands->PopDebugGroup();

    const auto blur_horizontal = [&blur_area, commands, grid_w, this] {
        commands->PushDebugGroup("CS Light BlurHorizontal");
        {
            commands->SetPipelineState(*m_light_horizontal_pipeline);
            commands->SetResourceHeap(*m_light_blur_resource_heap);
            commands->SetUniforms(0, &blur_area.min, sizeof(blur_area.min));
            commands->SetUniforms(1, &blur_area.max, sizeof(blur_area.max));
            commands->Dispatch(grid_w, 1, 1);
        }
        commands->PopDebugGroup();
    };

    const auto blur_vertical = [&blur_area, commands, grid_h, this] {
        commands->PushDebugGroup("CS Light BlurVertical");
        {
            commands->SetPipelineState(*m_light_vertical_pipeline);
            commands->SetResourceHeap(*m_light_blur_resource_heap);
            commands->SetUniforms(0, &blur_area.min, sizeof(blur_area.min));
            commands->SetUniforms(1, &blur_area.max, sizeof(blur_area.max));
            commands->Dispatch(grid_h, 1, 1);
        }
        commands->PopDebugGroup();
    };

    for (int i = 0; i < 2; ++i) {
        blur_horizontal();  
        commands->ResourceBarrier(0, nullptr, 1, &m_light_texture);

        blur_vertical();
        commands->ResourceBarrier(0, nullptr, 1, &m_light_texture);
    }
    blur_horizontal();
}

void WorldRenderer::terminate() {
    const auto& context = m_renderer->Context();

    RESOURCE_RELEASE(m_pipeline);
    RESOURCE_RELEASE(m_light_set_light_sources_pipeline);
    RESOURCE_RELEASE(m_light_vertical_pipeline);
    RESOURCE_RELEASE(m_light_horizontal_pipeline);
    RESOURCE_RELEASE(m_light_init_resource_heap);
    RESOURCE_RELEASE(m_light_blur_resource_heap);
    RESOURCE_RELEASE(m_resource_heap);
    RESOURCE_RELEASE(m_tile_texture_data_buffer);
    RESOURCE_RELEASE(m_light_texture);
    RESOURCE_RELEASE(m_light_texture_target);
    RESOURCE_RELEASE(m_tile_texture);
}