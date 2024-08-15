#include "world_renderer.hpp"

#include <LLGL/Utils/Utility.h>
#include <glm/gtc/type_ptr.hpp>

#include "../assets.hpp"
#include "../world/chunk.hpp"

#include "LLGL/PipelineStateFlags.h"
#include "renderer.hpp"

struct __attribute__((aligned(16))) DepthUniformData {
    float tile_depth;
    float wall_depth;
};

void WorldRenderer::init() {
    const auto& context = Renderer::Context();

    auto depth_uniform = DepthUniformData {
        .tile_depth = 2,
        .wall_depth = 1
    };

    m_depth_buffer = context->CreateBuffer(LLGL::ConstantBufferDesc(sizeof(DepthUniformData)), &depth_uniform);

    const auto* render_pass = Renderer::DefaultRenderPass();
    const RenderBackend backend = Renderer::Backend();

    LLGL::PipelineLayoutDescriptor pipelineLayoutDesc;
    pipelineLayoutDesc.bindings = {
        LLGL::BindingDescriptor(
            "GlobalUniformBuffer",
            LLGL::ResourceType::Buffer,
            LLGL::BindFlags::ConstantBuffer,
            LLGL::StageFlags::VertexStage,
            LLGL::BindingSlot(1)
        ),
        LLGL::BindingDescriptor(
            "DepthBuffer",
            LLGL::ResourceType::Buffer,
            LLGL::BindFlags::ConstantBuffer,
            LLGL::StageFlags::VertexStage,
            LLGL::BindingSlot(2)
        ),
        LLGL::BindingDescriptor("u_texture_array", LLGL::ResourceType::Texture, LLGL::BindFlags::Sampled, LLGL::StageFlags::FragmentStage, LLGL::BindingSlot(3)),
        LLGL::BindingDescriptor("u_sampler", LLGL::ResourceType::Sampler, 0, LLGL::StageFlags::FragmentStage, backend.IsOpenGL() ? 3 : 4),
    };

    LLGL::PipelineLayout* pipelineLayout = context->CreatePipelineLayout(pipelineLayoutDesc);

    const ShaderPipeline& tilemap_shader = Assets::GetShader(ShaderAsset::TilemapShader);

    LLGL::GraphicsPipelineDescriptor pipelineDesc;
    pipelineDesc.debugName = "World Pipeline";
    pipelineDesc.vertexShader = tilemap_shader.vs;
    pipelineDesc.fragmentShader = tilemap_shader.ps;
    pipelineDesc.geometryShader = tilemap_shader.gs;
    pipelineDesc.pipelineLayout = pipelineLayout;
    pipelineDesc.indexFormat = LLGL::Format::Undefined;
    pipelineDesc.primitiveTopology = LLGL::PrimitiveTopology::TriangleStrip;
    pipelineDesc.renderPass = render_pass;
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

void WorldRenderer::render(const ChunkManager& chunk_manager) {
    auto* const commands = Renderer::CommandBuffer();

    commands->SetPipelineState(*m_pipeline);

    for (const glm::uvec2& pos : chunk_manager.visible_chunks()) {
        const RenderChunk& chunk = chunk_manager.render_chunks().at(pos);

        if (!chunk.walls_empty()) {
            const Texture& t = Assets::GetTexture(TextureAsset::Walls);

            commands->SetVertexBufferArray(*chunk.wall_buffer_array);
            commands->SetResource(0, *Renderer::GlobalUniformBuffer());
            commands->SetResource(1, *m_depth_buffer);
            commands->SetResource(2, *t.texture);
            commands->SetResource(3, Assets::GetSampler(t.sampler));

            commands->DrawInstanced(4, 0, chunk.walls_count);
        }

        if (!chunk.blocks_empty()) {
            const Texture& t = Assets::GetTexture(TextureAsset::Tiles);

            commands->SetVertexBufferArray(*chunk.block_buffer_array);
            commands->SetResource(0, *Renderer::GlobalUniformBuffer());
            commands->SetResource(1, *m_depth_buffer);
            commands->SetResource(2, *t.texture);
            commands->SetResource(3, Assets::GetSampler(t.sampler));

            commands->DrawInstanced(4, 0, chunk.blocks_count);
        }
    }
}

void WorldRenderer::terminate() {
    if (m_pipeline) Renderer::Context()->Release(*m_pipeline);
    if (m_depth_buffer) Renderer::Context()->Release(*m_depth_buffer);
}