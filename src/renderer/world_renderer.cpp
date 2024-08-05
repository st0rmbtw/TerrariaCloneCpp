#include "world_renderer.hpp"

#include <LLGL/Utils/Utility.h>
#include <glm/gtc/type_ptr.hpp>

#include "../assets.hpp"
#include "../world/chunk.hpp"

#include "renderer.hpp"

void WorldRenderer::init() {
    m_transform_buffer = Renderer::Context()->CreateBuffer(LLGL::ConstantBufferDesc(sizeof(glm::mat4)));

    const uint32_t samplerBinding = Renderer::Backend().IsOpenGL() ? 3 : 4;

    LLGL::PipelineLayoutDescriptor pipelineLayoutDesc;
    pipelineLayoutDesc.bindings = {
        LLGL::BindingDescriptor(
            "UniformBuffer",
            LLGL::ResourceType::Buffer,
            LLGL::BindFlags::ConstantBuffer,
            LLGL::StageFlags::GeometryStage,
            LLGL::BindingSlot(1)
        ),
        LLGL::BindingDescriptor(
            "TransformBuffer",
            LLGL::ResourceType::Buffer,
            LLGL::BindFlags::ConstantBuffer,
            LLGL::StageFlags::GeometryStage,
            LLGL::BindingSlot(2)
        ),
        LLGL::BindingDescriptor("u_texture_array", LLGL::ResourceType::Texture, LLGL::BindFlags::Sampled, LLGL::StageFlags::FragmentStage, LLGL::BindingSlot(3)),
        LLGL::BindingDescriptor("u_sampler", LLGL::ResourceType::Sampler, 0, LLGL::StageFlags::FragmentStage, samplerBinding),
    };

    LLGL::PipelineLayout* pipelineLayout = Renderer::Context()->CreatePipelineLayout(pipelineLayoutDesc);

    const ShaderPipeline& tilemap_shader = Assets::GetShader(ShaderAssetKey::TilemapShader);

    LLGL::GraphicsPipelineDescriptor pipelineDesc;
    pipelineDesc.debugName = "World Pipeline";
    pipelineDesc.vertexShader = tilemap_shader.vs;
    pipelineDesc.fragmentShader = tilemap_shader.ps;
    pipelineDesc.geometryShader = tilemap_shader.gs;
    pipelineDesc.pipelineLayout = pipelineLayout;
    pipelineDesc.indexFormat = LLGL::Format::Undefined;
    pipelineDesc.primitiveTopology = LLGL::PrimitiveTopology::PointList;
    pipelineDesc.renderPass = Renderer::SwapChain()->GetRenderPass();
    pipelineDesc.rasterizer.frontCCW = true;
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

    std::vector<const RenderChunk*> blocks;
    blocks.reserve(world.visible_chunks().size());

    std::vector<const RenderChunk*> walls;
    walls.reserve(world.visible_chunks().size());

    for (const glm::uvec2& pos : world.visible_chunks()) {
        const RenderChunk& chunk = world.render_chunks().at(pos);

        if (!chunk.blocks_empty()) blocks.push_back(&chunk);
        if (!chunk.walls_empty()) walls.push_back(&chunk);
    }

    for (const RenderChunk* chunk : walls) {
        commands->UpdateBuffer(*m_transform_buffer, 0, glm::value_ptr(chunk->transform_matrix), sizeof(glm::mat4));

        const Texture& t = Assets::GetTexture(TextureKey::Walls);

        commands->SetPipelineState(*m_pipeline);
        commands->SetVertexBuffer(*chunk->wall_vertex_buffer);
        commands->SetResource(0, *Renderer::ProjectionsUniformBuffer());
        commands->SetResource(1, *m_transform_buffer);
        commands->SetResource(2, *t.texture);
        commands->SetResource(3, Assets::GetSampler(t.sampler));

        commands->Draw(chunk->walls_count, 0);
    }

    for (const RenderChunk* chunk : blocks) {
        commands->UpdateBuffer(*m_transform_buffer, 0, glm::value_ptr(chunk->transform_matrix), sizeof(glm::mat4));

        const Texture& t = Assets::GetTexture(TextureKey::Tiles);

        commands->SetPipelineState(*m_pipeline);
        commands->SetVertexBuffer(*chunk->block_vertex_buffer);
        commands->SetResource(0, *Renderer::ProjectionsUniformBuffer());
        commands->SetResource(1, *m_transform_buffer);
        commands->SetResource(2, *t.texture);
        commands->SetResource(3, Assets::GetSampler(t.sampler));

        commands->Draw(chunk->blocks_count, 0);
    }
}

void WorldRenderer::terminate() {
    if (m_transform_buffer) Renderer::Context()->Release(*m_transform_buffer);
    if (m_pipeline) Renderer::Context()->Release(*m_pipeline);
}