#include "world_renderer.hpp"

#include <LLGL/Utils/Utility.h>
#include <LLGL/PipelineStateFlags.h>
#include <LLGL/ResourceHeapFlags.h>
#include <LLGL/TextureFlags.h>
#include <LLGL/Types.h>
#include <LLGL/PipelineLayoutFlags.h>
#include <LLGL/ResourceFlags.h>
#include <LLGL/ShaderFlags.h>
#include <LLGL/Container/DynamicArray.h>

#include <SGE/engine.hpp>
#include <SGE/renderer/macros.hpp>
#include <SGE/types/binding_layout.hpp>
#include <SGE/defines.hpp>

#include <glm/gtc/type_ptr.hpp>

#include <tracy/Tracy.hpp>

#include "../assets.hpp"
#include "../world/chunk.hpp"
#include "../world/utils.hpp"

#include "dynamic_lighting.hpp"
#include "types.hpp"
#include "utils.hpp"

static constexpr uint32_t LIGHTMAP_CHUNK_TILE_SIZE = 500;
static constexpr uint32_t LIGHTMAP_CHUNK_SIZE = LIGHTMAP_CHUNK_TILE_SIZE * Constants::SUBDIVISION;
static constexpr float LIGHTMAP_CHUNK_WORLD_SIZE = LIGHTMAP_CHUNK_TILE_SIZE * Constants::TILE_SIZE;
static constexpr float LIGHTMAP_TO_WORLD = Constants::TILE_SIZE / Constants::SUBDIVISION;

static constexpr float TILE_DEPTH = 0.3f;
static constexpr float WALL_DEPTH = 0.1f;

struct SGE_ALIGN(16) TileTextureData {
    glm::vec2 tex_size;
    glm::vec2 tex_padding;
    glm::vec2 tex_offset;
    glm::vec2 size;
    glm::vec2 offset;
    float depth;
};

void WorldRenderer::init() {
    ZoneScopedN("WorldRenderer::init");

    using Constants::TILE_SIZE;
    using Constants::WALL_SIZE;
    using Constants::TORCH_SIZE;

    m_renderer = &sge::Engine::Renderer();

    const auto& context = m_renderer->Context();

    const uint32_t samples = m_renderer->SwapChain()->GetSamples();

    {
        const glm::vec2 tile_tex_size = glm::vec2(Assets::GetTexture(TextureAsset::Tiles).size());
        const glm::vec2 tile_padding = glm::vec2(Constants::TILE_TEXTURE_PADDING) / tile_tex_size;
        const glm::vec2 tile_offset = glm::vec2(Constants::TILE_TEXTURE_PADDING) / tile_tex_size;

        const glm::vec2 wall_tex_size = glm::vec2(Assets::GetTexture(TextureAsset::Walls).size());
        const glm::vec2 wall_padding = glm::vec2(Constants::WALL_TEXTURE_PADDING) / wall_tex_size;

        const glm::vec2 trees_size = Assets::GetTextureAtlas(TextureAsset::Tiles5).size();
        const glm::vec2 tree_tops_size = Assets::GetTextureAtlas(TextureAsset::TreeTops).size();
        const glm::vec2 tree_branches_size = Assets::GetTextureAtlas(TextureAsset::TreeBranches).size();

        TileTextureData texture_data[TileType::Count];
        texture_data[TileType::Block] = TileTextureData{tile_tex_size, tile_padding, glm::vec2(0.0f), glm::vec2(TILE_SIZE),  glm::vec2(0.0f), TILE_DEPTH}; // Tile
        texture_data[TileType::Wall] = TileTextureData{wall_tex_size, wall_padding, glm::vec2(0.0f), glm::vec2(WALL_SIZE),  glm::vec2(-TILE_SIZE * 0.5f), WALL_DEPTH}; // Wall
        texture_data[TileType::Torch] = TileTextureData{tile_tex_size, tile_padding, glm::vec2(0.0f), glm::vec2(TORCH_SIZE), glm::vec2(-2.0f, 0.0f), TILE_DEPTH}; // Torch
        texture_data[TileType::Tree] = TileTextureData{tile_tex_size, tile_padding, tile_offset, trees_size, glm::vec2(0.0f, 0.0f), TILE_DEPTH}; // Tree
        texture_data[TileType::TreeTop] = TileTextureData{tile_tex_size, glm::vec2(0.0f), glm::vec2(0.0f), tree_tops_size, -tree_tops_size * 0.5f + glm::vec2(10.0f, 10.0f), TILE_DEPTH}; // Tree tops
        texture_data[TileType::TreeBranch] = TileTextureData{tile_tex_size, tile_padding, tile_offset, tree_branches_size, glm::vec2(-15.0f), TILE_DEPTH}; // Tree branches

        LLGL::BufferDescriptor desc;
        desc.size = sizeof(texture_data);
        desc.bindFlags = LLGL::BindFlags::ConstantBuffer;

        m_tile_texture_data_buffer = context->CreateBuffer(desc, texture_data);
    }

    {
        LLGL::PipelineLayoutDescriptor pipelineLayoutDesc;
        pipelineLayoutDesc.heapBindings = sge::BindingLayout(
            LLGL::StageFlags::VertexStage,
            {
                sge::BindingLayoutItem::ConstantBuffer(2, "GlobalUniformBuffer"),
                sge::BindingLayoutItem::ConstantBuffer(3, "TileDataBuffer"),
            }
        );
        pipelineLayoutDesc.bindings = {
            LLGL::BindingDescriptor("u_texture_array", LLGL::ResourceType::Texture, LLGL::BindFlags::Sampled, LLGL::StageFlags::FragmentStage, LLGL::BindingSlot(4)),
        };
        pipelineLayoutDesc.staticSamplers = {
            LLGL::StaticSamplerDescriptor("u_sampler", LLGL::StageFlags::FragmentStage, 5, Assets::GetSampler(sge::TextureSampler::Nearest).descriptor()),
        };
        pipelineLayoutDesc.combinedTextureSamplers = {
            LLGL::CombinedTextureSamplerDescriptor{ "u_texture_array", "u_texture_array", "u_sampler", 4 }
        };

        LLGL::PipelineLayout* pipelineLayout = context->CreatePipelineLayout(pipelineLayoutDesc);

        const LLGL::ResourceViewDescriptor resource_views[] = {
            m_renderer->GlobalUniformBuffer(), m_tile_texture_data_buffer
        };

        m_resource_heap = context->CreateResourceHeap(pipelineLayout, resource_views);

        const sge::ShaderPipeline& tilemap_shader = Assets::GetShader(ShaderAsset::TilemapShader);

        LLGL::GraphicsPipelineDescriptor pipelineDesc;
        pipelineDesc.debugName = "World Pipeline";
        pipelineDesc.vertexShader = tilemap_shader.vs;
        pipelineDesc.fragmentShader = tilemap_shader.ps;
        pipelineDesc.geometryShader = tilemap_shader.gs;
        pipelineDesc.pipelineLayout = pipelineLayout;
        pipelineDesc.renderPass = m_render_pass;
        pipelineDesc.indexFormat = LLGL::Format::R16UInt;
        pipelineDesc.primitiveTopology = LLGL::PrimitiveTopology::TriangleStrip;
        pipelineDesc.rasterizer.frontCCW = true;
        pipelineDesc.rasterizer.multiSampleEnabled = (samples > 1);
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

    {
        LLGL::PipelineLayoutDescriptor lightmapPipelineLayoutDesc;
        lightmapPipelineLayoutDesc.heapBindings = sge::BindingLayout({
            sge::BindingLayoutItem::ConstantBuffer(2, "GlobalUniformBuffer", LLGL::StageFlags::VertexStage)
        });
        lightmapPipelineLayoutDesc.bindings = sge::BindingLayout({
            sge::BindingLayoutItem::Texture(3, "u_texture", LLGL::StageFlags::FragmentStage)
        });
        lightmapPipelineLayoutDesc.staticSamplers = {
            LLGL::StaticSamplerDescriptor("u_sampler", LLGL::StageFlags::FragmentStage, LLGL::BindingSlot(4), Assets::GetSampler(sge::TextureSampler::Nearest).descriptor())
        };
        lightmapPipelineLayoutDesc.combinedTextureSamplers = {
            LLGL::CombinedTextureSamplerDescriptor{ "u_texture", "u_texture", "u_sampler", 3 }
        };

        LLGL::PipelineLayout* lightmapPipelineLayout = context->CreatePipelineLayout(lightmapPipelineLayoutDesc);

        const LLGL::ResourceViewDescriptor lightmapResourceViews[] = {
            m_renderer->GlobalUniformBuffer()
        };

        m_lightmap_resource_heap = context->CreateResourceHeap(lightmapPipelineLayout, lightmapResourceViews);

        const sge::ShaderPipeline lightmap_shader = Assets::GetShader(ShaderAsset::StaticLightMapShader);

        LLGL::GraphicsPipelineDescriptor lightPipelineDesc;
        lightPipelineDesc.debugName = "WorldStaticLightMapPipeline";
        lightPipelineDesc.vertexShader = lightmap_shader.vs;
        lightPipelineDesc.fragmentShader = lightmap_shader.ps;
        lightPipelineDesc.pipelineLayout = lightmapPipelineLayout;
        lightPipelineDesc.indexFormat = LLGL::Format::R16UInt;
        lightPipelineDesc.primitiveTopology = LLGL::PrimitiveTopology::TriangleStrip;
        lightPipelineDesc.rasterizer.frontCCW = true;
        lightPipelineDesc.rasterizer.multiSampleEnabled = (samples > 1);
        lightPipelineDesc.renderPass = m_static_lightmap_render_pass;
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

void WorldRenderer::init_lighting(const WorldData& world) {
    if (SupportsAcceleratedDynamicLighting(*m_renderer)) {
        m_dynamic_lighting = std::make_unique<AcceleratedDynamicLighting>(world, m_light_texture);
    } else {
        m_dynamic_lighting = std::make_unique<DynamicLighting>(world, m_light_texture);
    }
}

void WorldRenderer::init_targets(LLGL::Extent2D resolution) {
    const auto& context = m_renderer->Context();
    const LLGL::SwapChain* swap_chain = m_renderer->SwapChain();

    const uint32_t samples = swap_chain->GetSamples();

    SGE_RESOURCE_RELEASE(m_target);
    SGE_RESOURCE_RELEASE(m_target_texture);
    SGE_RESOURCE_RELEASE(m_depth_texture);

    SGE_RESOURCE_RELEASE(m_static_lightmap_target);
    SGE_RESOURCE_RELEASE(m_static_lightmap_texture);

    LLGL::TextureDescriptor texture_desc;
    texture_desc.miscFlags = LLGL::MiscFlags::FixedSamples;
    texture_desc.cpuAccessFlags = 0;
    texture_desc.mipLevels = 1;

    texture_desc.extent.width = resolution.width;
    texture_desc.extent.height = resolution.height;
    texture_desc.format = swap_chain->GetColorFormat();
    texture_desc.bindFlags = LLGL::BindFlags::Sampled | LLGL::BindFlags::ColorAttachment;
    m_target_texture = context->CreateTexture(texture_desc);
    m_static_lightmap_texture = context->CreateTexture(texture_desc);

    LLGL::TextureDescriptor depth_texture_desc = texture_desc;
    depth_texture_desc.type = samples > 1 ? LLGL::TextureType::Texture2DMS : LLGL::TextureType::Texture2D;
    depth_texture_desc.samples = samples;
    depth_texture_desc.extent.width = resolution.width;
    depth_texture_desc.extent.height = resolution.height;
    depth_texture_desc.format = swap_chain->GetDepthStencilFormat();
    depth_texture_desc.bindFlags = LLGL::BindFlags::Sampled | LLGL::BindFlags::DepthStencilAttachment;
    m_depth_texture = context->CreateTexture(depth_texture_desc);

    if (m_render_pass == nullptr) {
        LLGL::RenderPassDescriptor render_pass;
        render_pass.colorAttachments[0].loadOp = LLGL::AttachmentLoadOp::Load;
        render_pass.colorAttachments[0].storeOp = LLGL::AttachmentStoreOp::Store;
        render_pass.colorAttachments[0].format = texture_desc.format;
        render_pass.depthAttachment.format = swap_chain->GetDepthStencilFormat();
        render_pass.depthAttachment.storeOp = LLGL::AttachmentStoreOp::Store;
        render_pass.stencilAttachment.format = swap_chain->GetDepthStencilFormat();
        render_pass.stencilAttachment.storeOp = LLGL::AttachmentStoreOp::Store;
        render_pass.samples = samples;
        m_render_pass = context->CreateRenderPass(render_pass);
    }

    if (m_static_lightmap_render_pass == nullptr) {
        LLGL::RenderPassDescriptor render_pass_desc;
        render_pass_desc.colorAttachments[0].loadOp = LLGL::AttachmentLoadOp::Undefined;
        render_pass_desc.colorAttachments[0].storeOp = LLGL::AttachmentStoreOp::Store;
        render_pass_desc.colorAttachments[0].format = swap_chain->GetColorFormat();
        render_pass_desc.samples = samples;
        m_static_lightmap_render_pass = context->CreateRenderPass(render_pass_desc);
    }

    {
        LLGL::RenderTargetDescriptor target_desc;
        target_desc.renderPass = m_render_pass;
        target_desc.resolution = resolution;
        target_desc.samples = samples;
        target_desc.depthStencilAttachment = m_depth_texture;
        if (samples > 1) {
            target_desc.colorAttachments[0] = m_target_texture->GetFormat();
            target_desc.resolveAttachments[0] = m_target_texture;
        } else {
            target_desc.colorAttachments[0] = m_target_texture;
        }
        m_target = context->CreateRenderTarget(target_desc);
    }
    {
        LLGL::RenderTargetDescriptor target_desc;
        target_desc.renderPass = m_static_lightmap_render_pass;
        target_desc.resolution = resolution;
        target_desc.samples = samples;
        if (samples > 1) {
            target_desc.colorAttachments[0] = m_static_lightmap_texture->GetFormat();
            target_desc.resolveAttachments[0] = m_static_lightmap_texture;
        } else {
            target_desc.colorAttachments[0] = m_static_lightmap_texture;
        }
        m_static_lightmap_target = context->CreateRenderTarget(target_desc);
    }
}

void WorldRenderer::init_textures(const WorldData& world) {
    ZoneScopedN("WorldRenderer::init_textures");

    using Constants::SUBDIVISION;

    auto& context = m_renderer->Context();

    SGE_RESOURCE_RELEASE(m_light_texture);
    SGE_RESOURCE_RELEASE(m_light_texture_target);

    {
        LLGL::TextureDescriptor light_texture_desc;
        light_texture_desc.type      = LLGL::TextureType::Texture2D;
        light_texture_desc.format    = LLGL::Format::RGBA8UNorm;
        light_texture_desc.extent    = LLGL::Extent3D(world.lightmap.width, world.lightmap.height, 1);
        light_texture_desc.miscFlags = 0;
        light_texture_desc.bindFlags = LLGL::BindFlags::Storage | LLGL::BindFlags::Sampled | LLGL::BindFlags::ColorAttachment;
        light_texture_desc.mipLevels = 1;

        LLGL::DynamicArray<uint8_t> pixels(world.lightmap.width * world.lightmap.height * 3);

        LLGL::ImageView image_view;
        image_view.format   = LLGL::ImageFormat::RGB;
        image_view.dataType = LLGL::DataType::UInt8;
        image_view.data     = pixels.data();
        image_view.dataSize = pixels.size();

        m_light_texture = context->CreateTexture(light_texture_desc, &image_view);
    }

    LLGL::RenderTargetDescriptor lightTextureRenderTarget;
    lightTextureRenderTarget.resolution.width = world.lightmap.width;
    lightTextureRenderTarget.resolution.height = world.lightmap.height;
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

            Color* buffer = sge::checked_alloc<Color>(chunk_size.x * chunk_size.y);
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

SGE_FORCE_INLINE static void internal_update_world_lightmap(const WorldData& world, const LightMapTaskResult& result) {
    for (int y = 0; y < result.height; ++y) {
        memcpy(&world.lightmap.colors[(result.offset_y + y) * world.lightmap.width + result.offset_x], &result.data[y * result.width], result.width * sizeof(Color));
        memcpy(&world.lightmap.masks[(result.offset_y + y) * world.lightmap.width + result.offset_x], &result.mask[y * result.width], result.width * sizeof(LightMask));
    }
}

void WorldRenderer::update(World& world) {
    update_lightmap_texture(world.data());
    m_dynamic_lighting->update(world);
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

void WorldRenderer::render(const ChunkManager& chunk_manager) {
    ZoneScopedN("WorldRenderer::render");

    auto* const commands = m_renderer->CommandBuffer();

    commands->SetPipelineState(*m_pipeline);

    const sge::Texture& walls_texture = Assets::GetTexture(TextureAsset::Walls);
    const sge::Texture& tiles_texture = Assets::GetTexture(TextureAsset::Tiles);

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

static sge::URect get_chunk_range(const sge::Rect& camera_fov, glm::uvec2 lightmap_size) {
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

void WorldRenderer::render_lightmap(const sge::Camera& camera) {
    const sge::Rect camera_fov = utils::get_camera_fov(camera);
    const sge::URect chunk_range = get_chunk_range(camera_fov, glm::uvec2(m_lightmap_width, m_lightmap_height));

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

void WorldRenderer::compute_light(const sge::Camera& camera, const World& world) {
    m_dynamic_lighting->compute_light(camera, world);
}

void WorldRenderer::terminate() {
    const auto& context = m_renderer->Context();

    if (m_dynamic_lighting)
        m_dynamic_lighting->destroy();

    SGE_RESOURCE_RELEASE(m_pipeline);
    SGE_RESOURCE_RELEASE(m_resource_heap);
    SGE_RESOURCE_RELEASE(m_tile_texture_data_buffer);
    SGE_RESOURCE_RELEASE(m_light_texture);
    SGE_RESOURCE_RELEASE(m_light_texture_target);
}