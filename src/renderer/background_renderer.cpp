#include "background_renderer.hpp"

#include <LLGL/ShaderFlags.h>

#include <tracy/Tracy.hpp>

#include "LLGL/PipelineLayoutFlags.h"
#include "renderer.hpp"
#include "utils.hpp"
#include "types.hpp"

#include "../log.hpp"

constexpr uint32_t MAX_QUADS = 500;

namespace BackgroundFlags {
    enum : uint8_t {
        IgnoreCameraZoom = 0,
    };
};

void BackgroundRenderer::init() {
    ZoneScopedN("BackgroundRenderer::init");

    const RenderBackend backend = Renderer::Backend();
    const auto& context = Renderer::Context();
    const auto* swap_chain = Renderer::SwapChain();

    const Texture& backgrounds_texture = Assets::GetTexture(TextureAsset::Backgrounds);

    m_buffer = new BackgroundInstance[MAX_QUADS];
    m_buffer_ptr = m_buffer;

    m_world_buffer = new BackgroundInstance[MAX_QUADS];
    m_world_buffer_ptr = m_world_buffer;

    const BackgroundVertex vertices[] = {
        BackgroundVertex(glm::vec2(0.0f, 0.0f), backgrounds_texture.size()),
        BackgroundVertex(glm::vec2(0.0f, 1.0f), backgrounds_texture.size()),
        BackgroundVertex(glm::vec2(1.0f, 0.0f), backgrounds_texture.size()),
        BackgroundVertex(glm::vec2(1.0f, 1.0f), backgrounds_texture.size()),
    };

    m_vertex_buffer = CreateVertexBufferInit(sizeof(vertices), vertices, Assets::GetVertexFormat(VertexFormatAsset::BackgroundVertex), "BackgroundRenderer VertexBuffer");
    m_instance_buffer = CreateVertexBuffer(MAX_QUADS * sizeof(BackgroundInstance), Assets::GetVertexFormat(VertexFormatAsset::BackgroundInstance), "BackgroundRenderer InstanceBuffer");
    m_world_instance_buffer = CreateVertexBuffer(MAX_QUADS * sizeof(BackgroundInstance), Assets::GetVertexFormat(VertexFormatAsset::BackgroundInstance), "BackgroundRenderer InstanceBuffer");

    {
        LLGL::Buffer* buffers[] = { m_vertex_buffer, m_instance_buffer };
        m_buffer_array = context->CreateBufferArray(2, buffers);
    }
    {
        LLGL::Buffer* buffers[] = { m_vertex_buffer, m_world_instance_buffer };
        m_world_buffer_array = context->CreateBufferArray(2, buffers);
    }

    LLGL::PipelineLayoutDescriptor pipelineLayoutDesc;
    pipelineLayoutDesc.heapBindings = {
        LLGL::BindingDescriptor(
            "GlobalUniformBuffer",
            LLGL::ResourceType::Buffer,
            LLGL::BindFlags::ConstantBuffer,
            LLGL::StageFlags::VertexStage,
            LLGL::BindingSlot(2)
        ),
        LLGL::BindingDescriptor("u_texture", LLGL::ResourceType::Texture, LLGL::BindFlags::Sampled, LLGL::StageFlags::FragmentStage, LLGL::BindingSlot(3)),
    };
    pipelineLayoutDesc.staticSamplers = {
        LLGL::StaticSamplerDescriptor("u_sampler", LLGL::StageFlags::FragmentStage, LLGL::BindingSlot(backend.IsOpenGL() ? 3 : 4), Assets::GetSampler(backgrounds_texture.sampler()).descriptor()),
    };

    LLGL::PipelineLayout* pipelineLayout = context->CreatePipelineLayout(pipelineLayoutDesc);

    const LLGL::ResourceViewDescriptor resource_views[] = {
        Renderer::GlobalUniformBuffer(), backgrounds_texture
    };
    m_resource_heap = context->CreateResourceHeap(pipelineLayout, resource_views);

    const ShaderPipeline& background_shader = Assets::GetShader(ShaderAsset::BackgroundShader);

    LLGL::GraphicsPipelineDescriptor pipelineDesc;
    pipelineLayoutDesc.debugName = "BackgroundRenderer Pipeline";
    pipelineDesc.vertexShader = background_shader.vs;
    pipelineDesc.fragmentShader = background_shader.ps;
    pipelineDesc.pipelineLayout = pipelineLayout;
    pipelineDesc.indexFormat = LLGL::Format::R16UInt;
    pipelineDesc.primitiveTopology = LLGL::PrimitiveTopology::TriangleStrip;
    pipelineDesc.renderPass = swap_chain->GetRenderPass();
    pipelineDesc.rasterizer.frontCCW = true;

    m_pipeline = context->CreatePipelineState(pipelineDesc);

    if (const LLGL::Report* report = m_pipeline->GetReport()) {
        if (report->HasErrors()) LOG_ERROR("%s", report->GetText());
    }
}

void BackgroundRenderer::draw_layer_internal(const BackgroundLayer& layer, BackgroundInstance** p_buffer) {
    ZoneScopedN("BackgroundRenderer::draw_layer");

    const glm::vec2 offset = layer.anchor().to_vec2();
    const glm::vec2 pos = layer.position() - layer.size() * offset;

    int flags = 0;
    flags |= layer.nonscale() << BackgroundFlags::IgnoreCameraZoom;

    BackgroundInstance* buffer = *p_buffer;

    buffer->position = pos;
    buffer->size = layer.size();
    buffer->tex_size = layer.texture_size();
    buffer->speed = layer.speed();
    buffer->flags = flags;
    buffer->id = layer.id();
    (*p_buffer)++;
}

void BackgroundRenderer::render() {
    if (m_layer_count == 0) return;

    ZoneScopedN("BackgroundRenderer::render");

    auto* const commands = Renderer::CommandBuffer();

    const ptrdiff_t size = (uint8_t*) m_buffer_ptr - (uint8_t*) m_buffer;
    commands->UpdateBuffer(*m_instance_buffer, 0, m_buffer, size);
    
    commands->SetVertexBufferArray(*m_buffer_array);

    commands->SetPipelineState(*m_pipeline);
    commands->SetResourceHeap(*m_resource_heap);

    commands->DrawInstanced(4, 0, m_layer_count);

    m_buffer_ptr = m_buffer;
    m_layer_count = 0;
}

void BackgroundRenderer::render_world() {
    if (m_world_layer_count == 0) return;

    ZoneScopedN("BackgroundRenderer::render_world");

    auto* const commands = Renderer::CommandBuffer();

    const ptrdiff_t size = (uint8_t*) m_world_buffer_ptr - (uint8_t*) m_world_buffer;
    commands->UpdateBuffer(*m_world_instance_buffer, 0, m_world_buffer, size);
    
    commands->SetVertexBufferArray(*m_world_buffer_array);

    commands->SetPipelineState(*m_pipeline);
    commands->SetResourceHeap(*m_resource_heap);

    commands->DrawInstanced(4, 0, m_world_layer_count);

    m_world_buffer_ptr = m_world_buffer;
    m_world_layer_count = 0;
}

void BackgroundRenderer::terminate() {
    const auto& context = Renderer::Context();

    if (m_vertex_buffer) context->Release(*m_vertex_buffer);
    if (m_instance_buffer) context->Release(*m_instance_buffer);
    if (m_world_instance_buffer) context->Release(*m_world_instance_buffer);
    if (m_world_buffer_array) context->Release(*m_world_buffer_array);
    if (m_buffer_array) context->Release(*m_buffer_array);
    if (m_pipeline) context->Release(*m_pipeline);

    delete[] m_buffer;
    delete[] m_world_buffer;
}