#include "background_renderer.hpp"

#include "LLGL/ShaderFlags.h"
#include "assets.hpp"
#include "renderer.hpp"
#include "utils.hpp"

#include "../log.hpp"

constexpr uint32_t MAX_QUADS = 500;
constexpr uint32_t MAX_INDICES = MAX_QUADS * 6;
constexpr uint32_t MAX_VERTICES = MAX_QUADS * 4;

void BackgroundRenderer::init() {
    const auto& context = Renderer::Context();
    const auto* swap_chain = Renderer::SwapChain();

    m_buffer = new BackgroundVertex[MAX_VERTICES];
    m_buffer_ptr = m_buffer;

    uint32_t indices[MAX_INDICES];
    uint32_t offset = 0;
    for (size_t i = 0; i < MAX_INDICES; i += 6) {
        indices[i + 0] = 0 + offset;
        indices[i + 1] = 1 + offset;
        indices[i + 2] = 2 + offset;
        indices[i + 3] = 2 + offset;
        indices[i + 4] = 3 + offset;
        indices[i + 5] = 1 + offset;
        offset += 4;
    }

    m_vertex_buffer = CreateVertexBuffer(MAX_VERTICES * sizeof(BackgroundVertex), BackgroundVertexFormat(), "BackgroundRenderer VertexBuffer");
    m_index_buffer = CreateIndexBuffer(indices, LLGL::Format::R32UInt, "BackgroundRenderer IndexBuffer");

    const uint32_t samplerBinding = Renderer::Backend().IsOpenGL() ? 2 : 3;

    LLGL::PipelineLayoutDescriptor pipelineLayoutDesc;
    pipelineLayoutDesc.bindings = {
        LLGL::BindingDescriptor(
            "GlobalUniformBuffer",
            LLGL::ResourceType::Buffer,
            LLGL::BindFlags::ConstantBuffer,
            LLGL::StageFlags::VertexStage | LLGL::StageFlags::FragmentStage,
            LLGL::BindingSlot(1)
        ),
        LLGL::BindingDescriptor("u_texture", LLGL::ResourceType::Texture, LLGL::BindFlags::Sampled, LLGL::StageFlags::FragmentStage, LLGL::BindingSlot(2)),
        LLGL::BindingDescriptor("u_sampler", LLGL::ResourceType::Sampler, 0, LLGL::StageFlags::FragmentStage, LLGL::BindingSlot(samplerBinding)),
    };

    LLGL::PipelineLayout* pipelineLayout = context->CreatePipelineLayout(pipelineLayoutDesc);

    const ShaderPipeline& background_shader = Assets::GetShader(ShaderAssetKey::BackgroundShader);

    LLGL::GraphicsPipelineDescriptor pipelineDesc;
    pipelineLayoutDesc.debugName = "BackgroundRenderer Pipeline";
    pipelineDesc.vertexShader = background_shader.vs;
    pipelineDesc.fragmentShader = background_shader.ps;
    pipelineDesc.pipelineLayout = pipelineLayout;
    pipelineDesc.indexFormat = LLGL::Format::R32UInt;
    pipelineDesc.primitiveTopology = LLGL::PrimitiveTopology::TriangleList;
    pipelineDesc.renderPass = swap_chain->GetRenderPass();
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

    m_pipeline = context->CreatePipelineState(pipelineDesc);

    if (const LLGL::Report* report = m_pipeline->GetReport()) {
        if (report->HasErrors()) LOG_ERROR("%s", report->GetText());
    }
}

void BackgroundRenderer::draw_layer(const BackgroundLayer& layer) {
    const glm::vec2 offset = layer.anchor().to_vec2();
    const glm::vec2 pos = layer.position() - layer.size() * offset;

    const glm::vec2 tex_size = glm::vec2(layer.texture().size);
    m_buffer_ptr->position = pos;
    m_buffer_ptr->uv = glm::vec2(0.0f);
    m_buffer_ptr->size = layer.size();
    m_buffer_ptr->tex_size = tex_size;
    m_buffer_ptr->speed = layer.speed();
    m_buffer_ptr->nonscale = layer.nonscale();
    m_buffer_ptr++;

    m_buffer_ptr->position = pos + glm::vec2(layer.size().x, 0.0f);
    m_buffer_ptr->uv = glm::vec2(1.0f, 0.0f);
    m_buffer_ptr->size = layer.size();
    m_buffer_ptr->tex_size = tex_size;
    m_buffer_ptr->speed = layer.speed();
    m_buffer_ptr->nonscale = layer.nonscale();
    m_buffer_ptr++;

    m_buffer_ptr->position = pos + glm::vec2(0.0f, layer.size().y);
    m_buffer_ptr->uv = glm::vec2(0.0f, 1.0f);
    m_buffer_ptr->size = layer.size();
    m_buffer_ptr->tex_size = tex_size;
    m_buffer_ptr->speed = layer.speed();
    m_buffer_ptr->nonscale = layer.nonscale();
    m_buffer_ptr++;

    m_buffer_ptr->position = pos + layer.size();
    m_buffer_ptr->uv = glm::vec2(1.0f);
    m_buffer_ptr->size = layer.size();
    m_buffer_ptr->tex_size = tex_size;
    m_buffer_ptr->speed = layer.speed();
    m_buffer_ptr->nonscale = layer.nonscale();
    m_buffer_ptr++;

    m_layers.push_back(LayerData {
        .texture = layer.texture(),
        .offset = static_cast<int>(m_layers.size() * 4)
    });
}

void BackgroundRenderer::render() {
    if (m_layers.empty()) return;
    
    auto* const commands = Renderer::CommandBuffer();

    const ptrdiff_t size = (uint8_t*) m_buffer_ptr - (uint8_t*) m_buffer;
    commands->UpdateBuffer(*m_vertex_buffer, 0, m_buffer, size);
    
    commands->SetVertexBuffer(*m_vertex_buffer);
    commands->SetIndexBuffer(*m_index_buffer);

    commands->SetPipelineState(*m_pipeline);
    commands->SetResource(0, *Renderer::GlobalUniformBuffer());

    for (const LayerData& layer : m_layers) {
        const Texture& t = layer.texture;

        commands->SetResource(1, *t.texture);
        commands->SetResource(2, Assets::GetSampler(t.sampler));
        commands->DrawIndexed(6, 0, layer.offset);
    }

    m_buffer_ptr = m_buffer;
    m_layers.clear();
}

void BackgroundRenderer::terminate() {
    const auto& context = Renderer::Context();

    if (m_vertex_buffer) context->Release(*m_vertex_buffer);
    if (m_index_buffer) context->Release(*m_index_buffer);
    if (m_pipeline) context->Release(*m_pipeline);
}