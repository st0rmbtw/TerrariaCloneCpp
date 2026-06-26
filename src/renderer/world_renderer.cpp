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
#include <SGE/renderer/types.hpp>
#include <SGE/renderer/macros.hpp>
#include <SGE/types/binding_layout.hpp>
#include <SGE/defines.hpp>
#include <SGE/profile.hpp>

#include <glm/gtc/type_ptr.hpp>

#include "../assets.hpp"
#include "../world/chunk.hpp"

#include "dynamic_lighting.hpp"
#include "utils.hpp"

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

WorldRenderer::WorldRenderer(const std::shared_ptr<sge::Renderer>& renderer) : m_renderer(renderer) {
    ZoneScoped;

    using Constants::TILE_SIZE;
    using Constants::WALL_SIZE;
    using Constants::TORCH_SIZE;

    const auto& render_context = renderer->GetRenderContext();

    {
        const glm::vec2 tile_tex_size = glm::vec2(Assets::GetTexture(TextureAsset::Tiles).size());
        const glm::vec2 tile_padding = glm::vec2(Constants::TILE_TEXTURE_PADDING) / tile_tex_size;
        const glm::vec2 tile_offset = glm::vec2(Constants::TILE_TEXTURE_PADDING) / tile_tex_size;

        const glm::vec2 wall_tex_size = glm::vec2(Assets::GetTexture(TextureAsset::Walls).size());
        const glm::vec2 wall_padding = glm::vec2(Constants::WALL_TEXTURE_PADDING) / wall_tex_size;

        const glm::vec2 trees_size = glm::vec2(Assets::GetTextureAtlas(TextureAsset::Tiles5).size());
        const glm::vec2 tree_tops_size = glm::vec2(Assets::GetTextureAtlas(TextureAsset::TreeTops).size());
        const glm::vec2 tree_branches_size = glm::vec2(Assets::GetTextureAtlas(TextureAsset::TreeBranches).size());

        TileTextureData texture_data[TileType::Count];
        texture_data[TileType::Block] = TileTextureData{tile_tex_size, tile_padding, glm::vec2(0.0f), glm::vec2(TILE_SIZE),  glm::vec2(0.0f), TILE_DEPTH}; // Tile
        texture_data[TileType::Wall] = TileTextureData{wall_tex_size, wall_padding, glm::vec2(0.0f), glm::vec2(WALL_SIZE),  glm::vec2(-TILE_SIZE * 0.5f), WALL_DEPTH}; // Wall
        texture_data[TileType::Torch] = TileTextureData{tile_tex_size, tile_padding, glm::vec2(0.0f), glm::vec2(TORCH_SIZE), glm::vec2(-2.0f, 0.0f), TILE_DEPTH}; // Torch
        texture_data[TileType::Tree] = TileTextureData{tile_tex_size, tile_padding, tile_offset, trees_size, glm::vec2(0.0f, 0.0f), TILE_DEPTH}; // Tree
        texture_data[TileType::TreeCrown] = TileTextureData{tile_tex_size, glm::vec2(0.0f), glm::vec2(0.0f), tree_tops_size, -tree_tops_size * 0.5f + glm::vec2(10.0f, 10.0f), TILE_DEPTH}; // Tree tops
        texture_data[TileType::TreeBranch] = TileTextureData{tile_tex_size, tile_padding, tile_offset, tree_branches_size, glm::vec2(-15.0f), TILE_DEPTH}; // Tree branches

        m_tile_texture_data_buffer = render_context->CreateConstantBuffer(texture_data);
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
            LLGL::BindingDescriptor("TextureArray", LLGL::ResourceType::Texture, LLGL::BindFlags::Sampled, LLGL::StageFlags::FragmentStage, LLGL::BindingSlot(4)),
        };
        pipelineLayoutDesc.staticSamplers = {
            LLGL::StaticSamplerDescriptor("Sampler", LLGL::StageFlags::FragmentStage, 5, Assets::GetSampler(sge::TextureSampler::Nearest)->descriptor()),
        };
        pipelineLayoutDesc.combinedTextureSamplers = {
            LLGL::CombinedTextureSamplerDescriptor{ "TextureArray", "TextureArray", "Sampler", 4 }
        };

        sge::Ref<LLGL::PipelineLayout> pipelineLayout = render_context->CreatePipelineLayout(pipelineLayoutDesc);

        m_resource_heap = render_context->CreateResourceHeap(pipelineLayout, {
            m_renderer->GlobalUniformBuffer().Get(), m_tile_texture_data_buffer.Get()
        });

        const sge::ShaderPipeline& tilemap_shader = Assets::GetShader(ShaderAsset::TilemapShader);

        sge::GraphicsPipelineConfig pipelineConfig;
        pipelineConfig.debugName = "World Pipeline";
        pipelineConfig.vertexShader = tilemap_shader.vs;
        pipelineConfig.pixelShader = tilemap_shader.ps;
        pipelineConfig.geometryShader = tilemap_shader.gs;
        pipelineConfig.layout = pipelineLayout;
        pipelineConfig.primitiveTopology = LLGL::PrimitiveTopology::TriangleStrip;
        pipelineConfig.depth = LLGL::DepthDescriptor {
            .testEnabled = true,
            .writeEnabled = true,
            .compareOp = LLGL::CompareOp::GreaterEqual
        };
        pipelineConfig.blend = LLGL::BlendDescriptor {
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

        m_pipeline = renderer->GetRenderContext()->CreatePipelineState(pipelineConfig);
    }

    {
        LLGL::PipelineLayoutDescriptor lightmapPipelineLayoutDesc;
        lightmapPipelineLayoutDesc.heapBindings = sge::BindingLayout({
            sge::BindingLayoutItem::ConstantBuffer(2, "GlobalUniformBuffer", LLGL::StageFlags::VertexStage)
        });
        lightmapPipelineLayoutDesc.bindings = sge::BindingLayout({
            sge::BindingLayoutItem::Texture(3, "StaticLightMapChunk", LLGL::StageFlags::FragmentStage)
        });
        lightmapPipelineLayoutDesc.staticSamplers = {
            LLGL::StaticSamplerDescriptor("Sampler", LLGL::StageFlags::FragmentStage, LLGL::BindingSlot(4), Assets::GetSampler(sge::TextureSampler::Nearest)->descriptor())
        };
        lightmapPipelineLayoutDesc.combinedTextureSamplers = {
            LLGL::CombinedTextureSamplerDescriptor{ "StaticLightMapChunk", "StaticLightMapChunk", "Sampler", 3 }
        };

        sge::Ref<LLGL::PipelineLayout> lightmapPipelineLayout = render_context->CreatePipelineLayout(lightmapPipelineLayoutDesc);

        m_lightmap_resource_heap = render_context->CreateResourceHeap(lightmapPipelineLayout, {
            m_renderer->GlobalUniformBuffer().Get()
        });

        const sge::ShaderPipeline lightmap_shader = Assets::GetShader(ShaderAsset::StaticLightMapShader);

        sge::GraphicsPipelineConfig lightPipelineConfig;
        lightPipelineConfig.debugName = "WorldStaticLightMapPipeline";
        lightPipelineConfig.vertexShader = lightmap_shader.vs;
        lightPipelineConfig.pixelShader = lightmap_shader.ps;
        lightPipelineConfig.layout = lightmapPipelineLayout;
        lightPipelineConfig.primitiveTopology = LLGL::PrimitiveTopology::TriangleStrip;
        lightPipelineConfig.blend = LLGL::BlendDescriptor {
            .targets = {
                LLGL::BlendTargetDescriptor {
                    .blendEnabled = false,
                }
            }
        };

        m_lightmap_pipeline = renderer->GetRenderContext()->CreatePipelineState(lightPipelineConfig);
    }
}

void WorldRenderer::init_lighting(const WorldData& world) {
    if (SupportsAcceleratedDynamicLighting(*m_renderer->GetRenderContext())) {
        m_dynamic_lighting = std::make_unique<AcceleratedDynamicLighting>(m_renderer, world, m_dynamic_light_texture);
    } else {
        m_dynamic_lighting = std::make_unique<DynamicLighting>(m_renderer, world, m_dynamic_light_texture);
    }
}

void WorldRenderer::init_targets(LLGL::Extent2D resolution) {
    const auto& context = m_renderer->GetRenderContext();

    context->DeleteRenderTarget(m_target);
    context->DeleteRenderTarget(m_static_lightmap_target);

    LLGL::TextureDescriptor texture_desc;
    texture_desc.miscFlags = LLGL::MiscFlags::FixedSamples;
    texture_desc.cpuAccessFlags = 0;
    texture_desc.mipLevels = 1;
    texture_desc.extent.width = resolution.width;
    texture_desc.extent.height = resolution.height;
    texture_desc.format = LLGL::Format::RGBA8UNorm;
    m_target_texture = context->CreateTexture(texture_desc);
    m_static_lightmap_texture = context->CreateTexture(texture_desc);

    if (!m_render_pass.IsValid()) {
        sge::RenderPassConfig renderPassConfig;
        renderPassConfig.colorAttachments[0].loadOp = LLGL::AttachmentLoadOp::Load;
        renderPassConfig.colorAttachments[0].storeOp = LLGL::AttachmentStoreOp::Store;
        renderPassConfig.colorAttachments[0].format = texture_desc.format;
        renderPassConfig.depthAttachment.format = LLGL::Format::D24UNormS8UInt;
        renderPassConfig.depthAttachment.storeOp = LLGL::AttachmentStoreOp::Store;
        renderPassConfig.stencilAttachment.format = LLGL::Format::D24UNormS8UInt;
        renderPassConfig.stencilAttachment.storeOp = LLGL::AttachmentStoreOp::Store;
        m_render_pass = context->CreateRenderPass(renderPassConfig);
    }

    if (!m_static_lightmap_render_pass.IsValid()) {
        sge::RenderPassConfig renderPassConfig;
        renderPassConfig.colorAttachments[0].loadOp = LLGL::AttachmentLoadOp::Undefined;
        renderPassConfig.colorAttachments[0].storeOp = LLGL::AttachmentStoreOp::Store;
        renderPassConfig.colorAttachments[0].format = LLGL::Format::RGBA8UNorm;
        m_static_lightmap_render_pass = context->CreateRenderPass(renderPassConfig);
    }

    {
        sge::RenderTargetConfig targetConfig;
        targetConfig.renderPass = m_render_pass;
        targetConfig.resolution = resolution;
        targetConfig.colorAttachments[0] = sge::AttachmentConfig(m_target_texture);
        targetConfig.depthStencilAttachment.format = LLGL::Format::D24UNormS8UInt;
        targetConfig.format = LLGL::Format::RGBA8UNorm;
        m_target = context->CreateRenderTarget(targetConfig);
    }
    {
        sge::RenderTargetConfig targetConfig;
        targetConfig.renderPass = m_static_lightmap_render_pass;
        targetConfig.resolution = resolution;
        targetConfig.colorAttachments[0] = sge::AttachmentConfig(m_static_lightmap_texture);
        targetConfig.format = LLGL::Format::RGBA8UNorm;
        m_static_lightmap_target = context->CreateRenderTarget(targetConfig);
    }
}

void WorldRenderer::init_textures(LLGL::Extent2D viewport) {
    ZoneScoped;

    using Constants::SUBDIVISION;

    auto& context = m_renderer->GetRenderContext();

    if (m_dynamic_light_texture_target.IsValid())
        context->Release(m_dynamic_light_texture_target);

    const uint32_t width = (viewport.width / 16) * (Constants::SUBDIVISION * Constants::CAMERA_MIN_ZOOM);
    const uint32_t height = (viewport.height / 16) * (Constants::SUBDIVISION * Constants::CAMERA_MIN_ZOOM);

    {
        LLGL::TextureDescriptor light_texture_desc;
        light_texture_desc.type      = LLGL::TextureType::Texture2D;
        light_texture_desc.format    = LLGL::Format::RGBA8UNorm;
        light_texture_desc.extent    = LLGL::Extent3D(width, height, 1);
        light_texture_desc.miscFlags = LLGL::MiscFlags::FixedSamples;
        light_texture_desc.bindFlags = LLGL::BindFlags::Storage | LLGL::BindFlags::Sampled | LLGL::BindFlags::ColorAttachment;
        light_texture_desc.mipLevels = 1;

        m_dynamic_light_texture = context->CreateTexture(light_texture_desc);
    }

    sge::RenderTargetConfig lightTextureRenderTargetConfig;
    lightTextureRenderTargetConfig.resolution.width = width;
    lightTextureRenderTargetConfig.resolution.height = height;
    lightTextureRenderTargetConfig.colorAttachments[0] = sge::AttachmentConfig(m_dynamic_light_texture);
    lightTextureRenderTargetConfig.format = LLGL::Format::RGBA8UNorm;
    m_dynamic_light_texture_target = context->CreateRenderTarget(lightTextureRenderTargetConfig);

    if (m_dynamic_lighting) {
        m_dynamic_lighting->set_light_texture(m_dynamic_light_texture);
    }
}

void WorldRenderer::update(World& world) {
    m_dynamic_lighting->update(world);
}


void WorldRenderer::render(const ChunkManager& chunk_manager) {
    ZoneScoped;

    const auto& commands = m_renderer->CommandBuffer();

    commands->SetPipelineState(m_renderer->GetRenderContext()->GetOrCreatePipeline(m_pipeline));

    const sge::Texture& walls_texture = Assets::GetTexture(TextureAsset::Walls);
    const sge::Texture& tiles_texture = Assets::GetTexture(TextureAsset::Tiles);

    for (const glm::uvec2& pos : chunk_manager.visible_chunks()) {
        const RenderChunk* chunk = chunk_manager.render_chunks().get(pos);
        if (chunk == nullptr) continue;

        if (chunk->wall_count() > 0) {
            commands->SetVertexBufferArray(*chunk->wall_buffer_array());
            commands->SetResource(0, walls_texture);
            commands->SetResourceHeap(*m_resource_heap);

            commands->DrawInstanced(4, 0, chunk->wall_count());
        }

        if (chunk->block_count() > 0) {
            commands->SetVertexBufferArray(*chunk->block_buffer_array());
            commands->SetResource(0, tiles_texture);
            commands->SetResourceHeap(*m_resource_heap);

            commands->DrawInstanced(4, 0, chunk->block_count());
        }
    }
}

void WorldRenderer::render_lightmap(const ChunkManager& chunk_manager) {
    const auto& commands = m_renderer->CommandBuffer();

    commands->SetPipelineState(m_renderer->GetRenderContext()->GetOrCreatePipeline(m_lightmap_pipeline));
    commands->SetResourceHeap(*m_lightmap_resource_heap);

    for (glm::uvec2 chunk_pos : chunk_manager.visible_light_chunks()) {
        const StaticLightMapChunk* lightmap_chunk = chunk_manager.light_chunks().get(chunk_pos);
        if (lightmap_chunk == nullptr) continue;

        commands->SetVertexBuffer(*lightmap_chunk->vertex_buffer);
        commands->SetResource(0, *lightmap_chunk->texture);
        commands->Draw(4, 0);
    }
}

WorldRenderer::~WorldRenderer() {
    const auto& context = m_renderer->GetRenderContext();

    context->DeletePipeline(m_pipeline);
    context->Release(m_dynamic_light_texture_target);
}
