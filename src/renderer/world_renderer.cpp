#include "world_renderer.hpp"

#include <LLGL/Utils/Utility.h>
#include <glm/gtc/type_ptr.hpp>

#include "../assets.hpp"
#include "../world/chunk.hpp"

#include "LLGL/PipelineStateFlags.h"
#include "renderer.hpp"

struct __attribute__((aligned(16))) UniformData {
    float tile_depth;
    float wall_depth;
};

void WorldRenderer::init() {
    m_order_buffer = Renderer::Context()->CreateBuffer(LLGL::ConstantBufferDesc(sizeof(UniformData)));

    const uint32_t samplerBinding = Renderer::Backend().IsOpenGL() ? 3 : 4;

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
            "OrderBuffer",
            LLGL::ResourceType::Buffer,
            LLGL::BindFlags::ConstantBuffer,
            LLGL::StageFlags::VertexStage,
            LLGL::BindingSlot(2)
        ),
        LLGL::BindingDescriptor("u_texture_array", LLGL::ResourceType::Texture, LLGL::BindFlags::Sampled, LLGL::StageFlags::FragmentStage, LLGL::BindingSlot(3)),
        LLGL::BindingDescriptor("u_sampler", LLGL::ResourceType::Sampler, 0, LLGL::StageFlags::FragmentStage, samplerBinding),
    };

    LLGL::PipelineLayout* pipelineLayout = Renderer::Context()->CreatePipelineLayout(pipelineLayoutDesc);

    const ShaderPipeline& tilemap_shader = Assets::GetShader(ShaderAsset::TilemapShader);

    LLGL::GraphicsPipelineDescriptor pipelineDesc;
    pipelineDesc.debugName = "World Pipeline";
    pipelineDesc.vertexShader = tilemap_shader.vs;
    pipelineDesc.fragmentShader = tilemap_shader.ps;
    pipelineDesc.geometryShader = tilemap_shader.gs;
    pipelineDesc.pipelineLayout = pipelineLayout;
    pipelineDesc.indexFormat = LLGL::Format::Undefined;
    pipelineDesc.primitiveTopology = LLGL::PrimitiveTopology::TriangleStrip;
    pipelineDesc.renderPass = Renderer::SwapChain()->GetRenderPass();
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
                .srcAlpha = LLGL::BlendOp::Zero,
                .dstAlpha = LLGL::BlendOp::One
            }
        }
    };

    m_pipeline = Renderer::Context()->CreatePipelineState(pipelineDesc);
}

void WorldRenderer::render(const World& world) {
    auto* const commands = Renderer::CommandBuffer();

    auto uniform = UniformData {
        .tile_depth = static_cast<float>(m_tile_depth),
        .wall_depth = static_cast<float>(m_wall_depth),
    };

    commands->UpdateBuffer(*m_order_buffer, 0, &uniform, sizeof(UniformData));

    for (const glm::uvec2& pos : world.visible_chunks()) {
        const RenderChunk& chunk = world.render_chunks().at(pos);

        if (!chunk.walls_empty()) {
            const Texture& t = Assets::GetTexture(TextureAsset::Walls);

            commands->SetPipelineState(*m_pipeline);
            commands->SetVertexBufferArray(*chunk.wall_buffer_array);
            commands->SetResource(0, *Renderer::GlobalUniformBuffer());
            commands->SetResource(1, *m_order_buffer);
            commands->SetResource(2, *t.texture);
            commands->SetResource(3, Assets::GetSampler(t.sampler));

            commands->DrawInstanced(4, 0, chunk.walls_count);
        }

        if (!chunk.blocks_empty()) {
            const Texture& t = Assets::GetTexture(TextureAsset::Tiles);

            commands->SetPipelineState(*m_pipeline);
            commands->SetVertexBufferArray(*chunk.block_buffer_array);
            commands->SetResource(0, *Renderer::GlobalUniformBuffer());
            commands->SetResource(1, *m_order_buffer);
            commands->SetResource(2, *t.texture);
            commands->SetResource(3, Assets::GetSampler(t.sampler));

            commands->DrawInstanced(4, 0, chunk.blocks_count);
        }
    }
}

void WorldRenderer::terminate() {
    if (m_pipeline) Renderer::Context()->Release(*m_pipeline);
}