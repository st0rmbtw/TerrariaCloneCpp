#include <LLGL/Utils/Utility.h>

#include "renderer/renderer.hpp"
#include "renderer/world_renderer.hpp"
#include "renderer/assets.hpp"
#include "assets.hpp"

void WorldRenderer::Init() {
    m_constant_buffer = Renderer::Context()->CreateBuffer(LLGL::ConstantBufferDesc(sizeof(TilemapUniforms)));

#ifdef BACKEND_OPENGL
    constexpr uint32_t samplerBinding = 1;
#else
    constexpr uint32_t samplerBinding = 2;
#endif

    LLGL::PipelineLayoutDescriptor pipelineLayoutDesc;
    pipelineLayoutDesc.bindings = {
        LLGL::BindingDescriptor(
            "UniformBuffer",
            LLGL::ResourceType::Buffer,
            LLGL::BindFlags::ConstantBuffer,
            LLGL::StageFlags::GeometryStage,
            0
        ),
        LLGL::BindingDescriptor("u_texture_array", LLGL::ResourceType::Texture, LLGL::BindFlags::Sampled, LLGL::StageFlags::FragmentStage, 1),
        LLGL::BindingDescriptor("u_sampler", LLGL::ResourceType::Sampler, 0, LLGL::StageFlags::FragmentStage, samplerBinding),
    };

    LLGL::PipelineLayout* pipelineLayout = Renderer::Context()->CreatePipelineLayout(pipelineLayoutDesc);

    const ShaderPipeline& tilemap_shader = Assets::GetShader(ShaderAssetKey::TilemapShader);

    LLGL::GraphicsPipelineDescriptor pipelineDesc;
    pipelineDesc.vertexShader = tilemap_shader.vs;
    pipelineDesc.fragmentShader = tilemap_shader.ps;
    pipelineDesc.geometryShader = tilemap_shader.gs;
    pipelineDesc.pipelineLayout = pipelineLayout;
    pipelineDesc.indexFormat = LLGL::Format::Undefined;
    pipelineDesc.primitiveTopology = LLGL::PrimitiveTopology::PointList;
    pipelineDesc.renderPass = Renderer::SwapChain()->GetRenderPass();
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

void WorldRenderer::Render(const World& world) {
    const auto commands = Renderer::CommandBuffer();

    for (const auto& entry : world.render_chunks()) {
        const RenderChunk& chunk = entry.second;

        if (chunk.walls_empty()) continue;

        TilemapUniforms uniforms = TilemapUniforms {
            .projection = m_projection_matrix,
            .view = m_view_matrix,
            .transform = chunk.transform_matrix,
        };
        commands->UpdateBuffer(*m_constant_buffer, 0, &uniforms, sizeof(uniforms));

        const Texture& t = Assets::GetTexture(AssetKey::TextureWalls);

        commands->SetPipelineState(*m_pipeline);
        commands->SetVertexBuffer(*chunk.wall_vertex_buffer);
        commands->SetResource(0, *m_constant_buffer);
        commands->SetResource(1, *t.texture);
        commands->SetResource(2, Assets::GetSampler(t.sampler));

        commands->Draw(chunk.walls_count, 0);
    }

    for (const auto& entry : world.render_chunks()) {
        const RenderChunk& chunk = entry.second;

        if (chunk.blocks_empty()) continue;

        TilemapUniforms uniforms = TilemapUniforms {
            .projection = m_projection_matrix,
            .view = m_view_matrix,
            .transform = chunk.transform_matrix,
        };
        commands->UpdateBuffer(*m_constant_buffer, 0, &uniforms, sizeof(uniforms));

        const Texture& t = Assets::GetTexture(AssetKey::TextureTiles);

        commands->SetPipelineState(*m_pipeline);
        commands->SetVertexBuffer(*chunk.block_vertex_buffer);
        commands->SetResource(0, *m_constant_buffer);
        commands->SetResource(1, *t.texture);
        commands->SetResource(2, Assets::GetSampler(t.sampler));

        commands->Draw(chunk.blocks_count, 0);
    }
}

void WorldRenderer::Terminate() {
    Renderer::Context()->Release(*m_constant_buffer);
    Renderer::Context()->Release(*m_pipeline);
}