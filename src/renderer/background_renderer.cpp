#include "background_renderer.hpp"

#include <LLGL/ShaderFlags.h>
#include <LLGL/PipelineLayoutFlags.h>

#include <tracy/Tracy.hpp>

#include <SGE/log.hpp>
#include <SGE/engine.hpp>
#include <SGE/renderer/macros.hpp>
#include <SGE/types/binding_layout.hpp>

#include "types.hpp"

constexpr uint32_t MAX_QUADS = 500;

namespace BackgroundFlags {
    enum : uint8_t {
        IgnoreCameraZoom = 0,
    };
};

void BackgroundRenderer::init() {
    ZoneScopedN("BackgroundRenderer::init");

    m_renderer = &sge::Engine::Renderer();

    const sge::RenderBackend backend = m_renderer->Backend();
    const auto& context = m_renderer->Context();
    const auto* swap_chain = m_renderer->SwapChain();

    const sge::Texture& backgrounds_texture = Assets::GetTexture(TextureAsset::Backgrounds);

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

    m_vertex_buffer = m_renderer->CreateVertexBufferInit(sizeof(vertices), vertices, Assets::GetVertexFormat(VertexFormatAsset::BackgroundVertex), "BackgroundRenderer VertexBuffer");
    m_instance_buffer = m_renderer->CreateVertexBuffer(MAX_QUADS * sizeof(BackgroundInstance), Assets::GetVertexFormat(VertexFormatAsset::BackgroundInstance), "BackgroundRenderer InstanceBuffer");
    m_world_instance_buffer = m_renderer->CreateVertexBuffer(MAX_QUADS * sizeof(BackgroundInstance), Assets::GetVertexFormat(VertexFormatAsset::BackgroundInstance), "BackgroundRenderer InstanceBuffer");

    {
        LLGL::Buffer* buffers[] = { m_vertex_buffer, m_instance_buffer };
        m_buffer_array = context->CreateBufferArray(2, buffers);
    }
    {
        LLGL::Buffer* buffers[] = { m_vertex_buffer, m_world_instance_buffer };
        m_world_buffer_array = context->CreateBufferArray(2, buffers);
    }

    LLGL::PipelineLayoutDescriptor pipelineLayoutDesc;
    pipelineLayoutDesc.heapBindings = sge::BindingLayout(
        LLGL::StageFlags::VertexStage | LLGL::StageFlags::FragmentStage,
        {
            sge::BindingLayoutItem::ConstantBuffer(2, "GlobalUniformBuffer"),
            sge::BindingLayoutItem::Texture(3, "u_texture"),
        }
    );
    pipelineLayoutDesc.staticSamplers = {
        LLGL::StaticSamplerDescriptor("u_sampler", LLGL::StageFlags::FragmentStage, LLGL::BindingSlot(backend.IsOpenGL() ? 3 : 4), backgrounds_texture.sampler().descriptor()),
    };
    pipelineLayoutDesc.combinedTextureSamplers = {
        LLGL::CombinedTextureSamplerDescriptor{ "u_texture", "u_texture", "u_sampler", 3 }
    };

    m_pipeline_layout = context->CreatePipelineLayout(pipelineLayoutDesc);

    const LLGL::ResourceViewDescriptor resource_views[] = {
        m_renderer->GlobalUniformBuffer(), backgrounds_texture
    };
    m_resource_heap = context->CreateResourceHeap(m_pipeline_layout, resource_views);

    const sge::ShaderPipeline& background_shader = Assets::GetShader(ShaderAsset::BackgroundShader);

    LLGL::RenderPassDescriptor render_pass;
    render_pass.colorAttachments[0].loadOp = LLGL::AttachmentLoadOp::Load;
    render_pass.colorAttachments[0].storeOp = LLGL::AttachmentStoreOp::Store;
    render_pass.colorAttachments[0].format = swap_chain->GetColorFormat();

    LLGL::GraphicsPipelineDescriptor pipelineDesc;
    pipelineDesc.debugName = "BackgroundRenderer Pipeline";
    pipelineDesc.vertexShader = background_shader.vs;
    pipelineDesc.fragmentShader = background_shader.ps;
    pipelineDesc.pipelineLayout = m_pipeline_layout;
    pipelineDesc.indexFormat = LLGL::Format::R16UInt;
    pipelineDesc.primitiveTopology = LLGL::PrimitiveTopology::TriangleStrip;
    pipelineDesc.renderPass = context->CreateRenderPass(render_pass);
    pipelineDesc.rasterizer.frontCCW = true;

    m_pipeline = context->CreatePipelineState(pipelineDesc);

    if (const LLGL::Report* report = m_pipeline->GetReport()) {
        if (report->HasErrors()) SGE_LOG_ERROR("%s", report->GetText());
    }
}

void BackgroundRenderer::init_targets(LLGL::Extent2D resolution) {
    const auto& context = m_renderer->Context();
    const auto* swap_chain = m_renderer->SwapChain();

    SGE_RESOURCE_RELEASE(m_background_render_target);
    SGE_RESOURCE_RELEASE(m_background_render_texture);

    LLGL::TextureDescriptor texture_desc;
    texture_desc.extent.width = resolution.width;
    texture_desc.extent.height = resolution.height;
    texture_desc.format = swap_chain->GetColorFormat();
    texture_desc.bindFlags = LLGL::BindFlags::Sampled | LLGL::BindFlags::ColorAttachment;
    texture_desc.miscFlags = 0;
    texture_desc.cpuAccessFlags = 0;
    texture_desc.mipLevels = 1;

    m_background_render_texture = context->CreateTexture(texture_desc);

    LLGL::RenderTargetDescriptor background_target_desc;
    background_target_desc.resolution = resolution;
    background_target_desc.colorAttachments[0] = m_background_render_texture;
    m_background_render_target = context->CreateRenderTarget(background_target_desc);
}

void BackgroundRenderer::init_world(WorldRenderer& world_renderer) {
    const auto& context = m_renderer->Context();

    const sge::ShaderPipeline& background_shader = Assets::GetShader(ShaderAsset::BackgroundShader);

    LLGL::GraphicsPipelineDescriptor pipelineDesc;
    pipelineDesc.debugName = "BackgroundRenderer Pipeline World";
    pipelineDesc.vertexShader = background_shader.vs;
    pipelineDesc.fragmentShader = background_shader.ps;
    pipelineDesc.pipelineLayout = m_pipeline_layout;
    pipelineDesc.indexFormat = LLGL::Format::R16UInt;
    pipelineDesc.primitiveTopology = LLGL::PrimitiveTopology::TriangleStrip;
    pipelineDesc.renderPass = world_renderer.render_pass();
    pipelineDesc.rasterizer.frontCCW = true;

    m_pipeline_world = context->CreatePipelineState(pipelineDesc);

    if (const LLGL::Report* report = m_pipeline_world->GetReport()) {
        if (report->HasErrors()) SGE_LOG_ERROR("%s", report->GetText());
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

    auto* const commands = m_renderer->CommandBuffer();

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

    auto* const commands = m_renderer->CommandBuffer();

    const ptrdiff_t size = (uint8_t*) m_world_buffer_ptr - (uint8_t*) m_world_buffer;
    commands->UpdateBuffer(*m_world_instance_buffer, 0, m_world_buffer, size);

    commands->SetVertexBufferArray(*m_world_buffer_array);

    commands->SetPipelineState(*m_pipeline_world);
    commands->SetResourceHeap(*m_resource_heap);

    commands->DrawInstanced(4, 0, m_world_layer_count);

    m_world_buffer_ptr = m_world_buffer;
    m_world_layer_count = 0;
}

void BackgroundRenderer::terminate() {
    const auto& context = m_renderer->Context();

    SGE_RESOURCE_RELEASE(m_vertex_buffer);
    SGE_RESOURCE_RELEASE(m_instance_buffer);
    SGE_RESOURCE_RELEASE(m_world_instance_buffer);
    SGE_RESOURCE_RELEASE(m_world_buffer_array);
    SGE_RESOURCE_RELEASE(m_buffer_array);
    SGE_RESOURCE_RELEASE(m_pipeline);

    delete[] m_buffer;
    delete[] m_world_buffer;
}