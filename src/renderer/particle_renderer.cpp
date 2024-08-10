#include "particle_renderer.hpp"

#include "LLGL/ShaderFlags.h"
#include "assets.hpp"
#include "renderer.hpp"
#include "utils.hpp"

#include "../log.hpp"

inline constexpr size_t get_particle_index(Particle::Type type, uint8_t variant) {
    ASSERT(variant <= 2, "Variant must be in range from 0 to 3");

    const size_t index = static_cast<uint8_t>(type);
    const size_t y = index / PARTICLES_ATLAS_COLUMNS;
    const size_t x = index % PARTICLES_ATLAS_COLUMNS;
    return (y * 3 + variant) * PARTICLES_ATLAS_COLUMNS + x;
}

void ParticleRenderer::init() {
    const auto& context = Renderer::Context();
    const auto* swap_chain = Renderer::SwapChain();

    m_buffer = new ParticleVertex[MAX_PARTICLES_COUNT];
    m_buffer_ptr = m_buffer;

    m_vertex_buffer = CreateVertexBuffer(MAX_PARTICLES_COUNT * sizeof(ParticleVertex), ParticleVertexFormat(), "ParticleRenderer VertexBuffer");

    const uint32_t samplerBinding = Renderer::Backend().IsOpenGL() ? 2 : 3;

    LLGL::PipelineLayoutDescriptor pipelineLayoutDesc;
    pipelineLayoutDesc.bindings = {
        LLGL::BindingDescriptor(
            "GlobalUniformBuffer",
            LLGL::ResourceType::Buffer,
            LLGL::BindFlags::ConstantBuffer,
            LLGL::StageFlags::GeometryStage,
            LLGL::BindingSlot(1)
        ),
        LLGL::BindingDescriptor("u_texture", LLGL::ResourceType::Texture, LLGL::BindFlags::Sampled, LLGL::StageFlags::FragmentStage, LLGL::BindingSlot(2)),
        LLGL::BindingDescriptor("u_sampler", LLGL::ResourceType::Sampler, 0, LLGL::StageFlags::FragmentStage, LLGL::BindingSlot(samplerBinding)),
    };

    LLGL::PipelineLayout* pipelineLayout = context->CreatePipelineLayout(pipelineLayoutDesc);

    const ShaderPipeline& particle_shader = Assets::GetShader(ShaderAssetKey::ParticleShader);

    LLGL::GraphicsPipelineDescriptor pipelineDesc;
    pipelineLayoutDesc.debugName = "ParticleRenderer Pipeline";
    pipelineDesc.vertexShader = particle_shader.vs;
    pipelineDesc.geometryShader = particle_shader.gs;
    pipelineDesc.fragmentShader = particle_shader.ps;
    pipelineDesc.pipelineLayout = pipelineLayout;
    pipelineDesc.indexFormat = LLGL::Format::Undefined;
    pipelineDesc.primitiveTopology = LLGL::PrimitiveTopology::PointList;
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

    m_atlas = Assets::GetTextureAtlas(TextureKey::Particles);
}

void ParticleRenderer::draw_particle(const glm::vec2& position, const glm::quat& rotation, float scale, Particle::Type type, uint8_t variant) {
    const math::Rect& rect = m_atlas.get_rect(get_particle_index(type, variant));

    m_buffer_ptr->position = position;
    m_buffer_ptr->rotation = rotation;
    m_buffer_ptr->uv = rect.min;
    m_buffer_ptr->tex_size = glm::vec2(m_atlas.texture().size);
    m_buffer_ptr->scale = scale;
    m_buffer_ptr++;

    m_particle_count++;
}

void ParticleRenderer::render() {
    if (m_particle_count == 0) return;

    auto* const commands = Renderer::CommandBuffer();

    const ptrdiff_t size = (uint8_t*) m_buffer_ptr - (uint8_t*) m_buffer;
    if (size <= (1 << 16)) {
        commands->UpdateBuffer(*m_vertex_buffer, 0, m_buffer, size);
    } else {
        Renderer::Context()->WriteBuffer(*m_vertex_buffer, 0, m_buffer, size);
    }
    
    commands->SetVertexBuffer(*m_vertex_buffer);

    commands->SetPipelineState(*m_pipeline);

    const Texture& t = m_atlas.texture();

    commands->SetResource(0, *Renderer::GlobalUniformBuffer());
    commands->SetResource(1, *t.texture);
    commands->SetResource(2, Assets::GetSampler(t.sampler));

    commands->Draw(m_particle_count, 0);

    m_buffer_ptr = m_buffer;
    m_particle_count = 0;
}

void ParticleRenderer::terminate() {
    const auto& context = Renderer::Context();

    if (m_vertex_buffer) context->Release(*m_vertex_buffer);
    if (m_pipeline) context->Release(*m_pipeline);

    delete[] m_buffer;
}