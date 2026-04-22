#include "background_renderer.hpp"

#include <LLGL/ShaderFlags.h>
#include <LLGL/PipelineLayoutFlags.h>

#include <SGE/log.hpp>
#include <SGE/engine.hpp>
#include <SGE/profile.hpp>
#include <SGE/renderer/macros.hpp>
#include <SGE/renderer/types.hpp>
#include <SGE/types/binding_layout.hpp>

#include "types.hpp"

constexpr uint32_t MAX_QUADS = 500;

namespace BackgroundFlags {
    enum : uint8_t {
        IgnoreCameraZoom = 0,
        UI
    };
};

BackgroundRenderer::BackgroundRenderer(const std::shared_ptr<sge::Renderer>& renderer) : m_renderer(renderer) {
    ZoneScoped;

    const sge::RenderBackend backend = m_renderer->GetRenderContext()->Backend();
    const auto& render_context = m_renderer->GetRenderContext();

    const sge::Texture& backgrounds_texture = Assets::GetTexture(TextureAsset::Backgrounds);

    m_buffer = HeapArray<BackgroundInstance>(MAX_QUADS);
    m_buffer_ptr = m_buffer.data();

    m_world_buffer = HeapArray<BackgroundInstance>(MAX_QUADS);
    m_world_buffer_ptr = m_world_buffer.data();

    const BackgroundVertex vertices[] = {
        BackgroundVertex(glm::vec2(0.0f, 0.0f), glm::vec2(backgrounds_texture.size())),
        BackgroundVertex(glm::vec2(0.0f, 1.0f), glm::vec2(backgrounds_texture.size())),
        BackgroundVertex(glm::vec2(1.0f, 0.0f), glm::vec2(backgrounds_texture.size())),
        BackgroundVertex(glm::vec2(1.0f, 1.0f), glm::vec2(backgrounds_texture.size())),
    };

    m_vertex_buffer = render_context->CreateVertexBuffer(vertices, Assets::GetVertexFormat(VertexFormatAsset::BackgroundVertex), "BackgroundRenderer VertexBuffer");
    m_instance_buffer = render_context->CreateVertexBuffer(MAX_QUADS * sizeof(BackgroundInstance), Assets::GetVertexFormat(VertexFormatAsset::BackgroundInstance), "BackgroundRenderer InstanceBuffer");
    m_world_instance_buffer = render_context->CreateVertexBuffer(MAX_QUADS * sizeof(BackgroundInstance), Assets::GetVertexFormat(VertexFormatAsset::BackgroundInstance), "BackgroundRenderer InstanceBuffer");

    m_buffer_array = render_context->CreateBufferArray({ m_vertex_buffer.Get(), m_instance_buffer.Get() });
    m_world_buffer_array = render_context->CreateBufferArray({ m_vertex_buffer.Get(), m_world_instance_buffer.Get() });

    LLGL::PipelineLayoutDescriptor pipelineLayoutDesc;
    pipelineLayoutDesc.heapBindings = sge::BindingLayout({
        sge::BindingLayoutItem::ConstantBuffer(2, "GlobalUniformBuffer", LLGL::StageFlags::VertexStage),
        sge::BindingLayoutItem::Texture(3, "Texture", LLGL::StageFlags::FragmentStage),
    });
    pipelineLayoutDesc.staticSamplers = {
        LLGL::StaticSamplerDescriptor("Sampler", LLGL::StageFlags::FragmentStage, LLGL::BindingSlot(backend.IsOpenGL() ? 3 : 4), backgrounds_texture.sampler()->descriptor()),
    };
    pipelineLayoutDesc.combinedTextureSamplers = {
        LLGL::CombinedTextureSamplerDescriptor{ "Texture", "Texture", "Sampler", 3 }
    };

    m_pipeline_layout = render_context->CreatePipelineLayout(pipelineLayoutDesc);
    m_resource_heap = render_context->CreateResourceHeap(m_pipeline_layout, {
        m_renderer->GlobalUniformBuffer().Get(),
        backgrounds_texture
    });

    const sge::ShaderPipeline& background_shader = Assets::GetShader(ShaderAsset::BackgroundShader);

    sge::GraphicsPipelineConfig pipelineConfig;
    pipelineConfig.debugName = "BackgroundRenderer Pipeline";
    pipelineConfig.vertexShader = background_shader.vs;
    pipelineConfig.pixelShader = background_shader.ps;
    pipelineConfig.layout = m_pipeline_layout;
    pipelineConfig.primitiveTopology = LLGL::PrimitiveTopology::TriangleStrip;

    m_pipeline = render_context->CreatePipelineState(pipelineConfig);
}

void BackgroundRenderer::init_targets(LLGL::Extent2D resolution) {
    const auto& context = m_renderer->GetRenderContext();

    context->DeleteRenderTarget(m_background_render_target);

    LLGL::TextureDescriptor texture_desc;
    texture_desc.extent.width = resolution.width;
    texture_desc.extent.height = resolution.height;
    texture_desc.format = LLGL::Format::RGBA8UNorm;
    texture_desc.bindFlags = LLGL::BindFlags::Sampled | LLGL::BindFlags::ColorAttachment;
    texture_desc.miscFlags = LLGL::MiscFlags::FixedSamples;
    texture_desc.cpuAccessFlags = 0;
    texture_desc.mipLevels = 1;
    m_background_render_texture = context->CreateTexture(texture_desc);

    sge::RenderTargetConfig backgroundTargetConfig;
    backgroundTargetConfig.resolution = resolution;
    backgroundTargetConfig.colorAttachments[0] = sge::AttachmentConfig(m_background_render_texture);
    backgroundTargetConfig.format = LLGL::Format::RGBA8UNorm;
    m_background_render_target = context->CreateRenderTarget(backgroundTargetConfig);
}

void BackgroundRenderer::init_world(WorldRenderer& world_renderer) {
    const auto& context = m_renderer->GetRenderContext();

    const sge::ShaderPipeline& background_shader = Assets::GetShader(ShaderAsset::BackgroundShader);

    sge::GraphicsPipelineConfig pipelineDesc;
    pipelineDesc.debugName = "BackgroundRenderer Pipeline World";
    pipelineDesc.vertexShader = background_shader.vs;
    pipelineDesc.pixelShader = background_shader.ps;
    pipelineDesc.layout = m_pipeline_layout;
    pipelineDesc.primitiveTopology = LLGL::PrimitiveTopology::TriangleStrip;
    pipelineDesc.renderPass = world_renderer.render_pass();

    m_pipeline_world = context->CreatePipelineState(pipelineDesc);
}

void BackgroundRenderer::reset() {
    m_buffer_ptr = m_buffer.data();
    m_layer_count = 0;
    m_world_buffer_ptr = m_world_buffer.data();
    m_world_layer_count = 0;
}

void BackgroundRenderer::draw_layer_internal(const BackgroundLayer& layer, BackgroundInstance** p_buffer) {
    ZoneScoped;

    const glm::vec2 offset = layer.anchor().to_vec2();
    const glm::vec2 pos = layer.position() - layer.size() * offset;

    uint32_t flags = 0;
    flags |= layer.nonscale() << BackgroundFlags::IgnoreCameraZoom;
    flags |= layer.is_ui() << BackgroundFlags::UI;

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
    ZoneScoped;

    if (m_layer_count == 0) return;

    const auto& commands = m_renderer->CommandBuffer();

    const ptrdiff_t size = (uint8_t*) m_buffer_ptr - (uint8_t*) m_buffer.data();
    commands->UpdateBuffer(*m_instance_buffer, 0, m_buffer.data(), size);

    commands->SetVertexBufferArray(*m_buffer_array);

    commands->SetPipelineState(m_renderer->GetRenderContext()->GetOrCreatePipeline(m_pipeline));
    commands->SetResourceHeap(*m_resource_heap);

    commands->DrawInstanced(4, 0, m_layer_count);

    m_buffer_ptr = m_buffer.data();
    m_layer_count = 0;
}

void BackgroundRenderer::render_world() {
    ZoneScoped;

    if (m_world_layer_count == 0) return;

    const auto& render_context = m_renderer->GetRenderContext();
    const auto& commands = m_renderer->CommandBuffer();

    const ptrdiff_t size = (uint8_t*) m_world_buffer_ptr - (uint8_t*) m_world_buffer.data();
    commands->UpdateBuffer(*m_world_instance_buffer, 0, m_world_buffer.data(), size);

    commands->SetVertexBufferArray(*m_world_buffer_array);

    commands->SetPipelineState(render_context->GetOrCreatePipeline(m_pipeline_world));
    commands->SetResourceHeap(*m_resource_heap);

    commands->DrawInstanced(4, 0, m_world_layer_count);

    m_world_buffer_ptr = m_world_buffer.data();
    m_world_layer_count = 0;
}
