#include "particle_renderer.hpp"

#include <LLGL/Format.h>
#include <LLGL/PipelineLayoutFlags.h>
#include <LLGL/ShaderFlags.h>

#include <SGE/engine.hpp>
#include <SGE/log.hpp>
#include <SGE/profile.hpp>
#include <SGE/renderer/macros.hpp>
#include <SGE/types/binding_layout.hpp>

#include "types.hpp"

#include "../assets.hpp"

using Constants::PARTICLE_SIZE;
using Constants::MAX_PARTICLES_COUNT;

static inline constexpr size_t get_particle_index(Particle::Type type, uint8_t variant) {
    SGE_ASSERT(variant <= 2);

    const size_t index = static_cast<uint8_t>(type);
    const size_t y = index / PARTICLES_ATLAS_COLUMNS;
    const size_t x = index % PARTICLES_ATLAS_COLUMNS;
    return (y * 3 + variant) * PARTICLES_ATLAS_COLUMNS + x;
}

ParticleRenderer::ParticleRenderer(const std::shared_ptr<sge::Renderer>& renderer) : m_renderer(renderer) {
    ZoneScoped;

    const auto& render_context = m_renderer->GetRenderContext();
    const sge::RenderBackend backend = render_context->Backend();

    m_atlas = Assets::GetTextureAtlas(TextureAsset::Particles);

    m_instance_buffer_data = sge::HeapArray<ParticleInstance>(MAX_PARTICLES_COUNT);
    m_instance_buffer_data_world = sge::HeapArray<ParticleInstance>(MAX_PARTICLES_COUNT);

    m_position_buffer_data = sge::HeapArray<glm::vec2>(MAX_PARTICLES_COUNT);
    m_rotation_buffer_data = sge::HeapArray<glm::quat>(MAX_PARTICLES_COUNT);
    m_scale_buffer_data = sge::HeapArray<float>(MAX_PARTICLES_COUNT);

    const glm::vec2 texture_size = glm::vec2(m_atlas.texture().size());

    const ParticleVertex vertices[] = {
        ParticleVertex(0.0, 0.0, PARTICLE_SIZE / texture_size, texture_size),
        ParticleVertex(0.0, 1.0, PARTICLE_SIZE / texture_size, texture_size),
        ParticleVertex(1.0, 0.0, PARTICLE_SIZE / texture_size, texture_size),
        ParticleVertex(1.0, 1.0, PARTICLE_SIZE / texture_size, texture_size),
    };

    m_vertex_buffer = render_context->CreateVertexBuffer(vertices, Assets::GetVertexFormat(VertexFormatAsset::ParticleVertex), "ParticleRenderer VertexBuffer");
    m_instance_buffer = render_context->CreateVertexBuffer(MAX_PARTICLES_COUNT * sizeof(ParticleInstance), Assets::GetVertexFormat(VertexFormatAsset::ParticleInstance), "ParticleRenderer InstanceBuffer");
    m_buffer_array = render_context->CreateBufferArray({ m_vertex_buffer.Get(), m_instance_buffer.Get() });

    {
        LLGL::BufferDescriptor bufferDesc;
        bufferDesc.debugName    = "Particle TransformBuffer";
        bufferDesc.size         = sizeof(glm::mat4) * MAX_PARTICLES_COUNT;
        bufferDesc.bindFlags    = LLGL::BindFlags::Storage | LLGL::BindFlags::Sampled;
        bufferDesc.format       = LLGL::Format::RGBA32Float;

        m_transform_buffer = render_context->CreateBuffer(bufferDesc);
    }

    m_position_buffer = render_context->CreateStructuredBuffer<glm::vec2>(MAX_PARTICLES_COUNT, "Particle PositionBuffer");
    m_rotation_buffer = render_context->CreateStructuredBuffer<glm::vec4>(MAX_PARTICLES_COUNT, "Particle RotationBuffer");
    m_scale_buffer = render_context->CreateStructuredBuffer<glm::vec2>(MAX_PARTICLES_COUNT, "Particle ScaleBuffer");

    LLGL::PipelineLayoutDescriptor pipelineLayoutDesc;
    pipelineLayoutDesc.heapBindings = sge::BindingLayout({
        sge::BindingLayoutItem::ConstantBuffer(2, "GlobalUniformBuffer", LLGL::StageFlags::VertexStage),
        sge::BindingLayoutItem::Buffer(5, "TransformBuffer", LLGL::StageFlags::VertexStage),
        sge::BindingLayoutItem::Texture(3, "Texture", LLGL::StageFlags::FragmentStage),
    });
    pipelineLayoutDesc.staticSamplers = {
        LLGL::StaticSamplerDescriptor("Sampler", LLGL::StageFlags::FragmentStage, LLGL::BindingSlot(4), m_atlas.texture().sampler()->descriptor())
    };
    pipelineLayoutDesc.combinedTextureSamplers = {
        LLGL::CombinedTextureSamplerDescriptor{ "Texture", "Texture", "Sampler", 3 }
    };

    sge::Ref<LLGL::PipelineLayout> pipelineLayout = render_context->CreatePipelineLayout(pipelineLayoutDesc);
    m_resource_heap = render_context->CreateResourceHeap(pipelineLayout, {
        m_renderer->GlobalUniformBuffer().Get(),
        m_transform_buffer.Get(),
        m_atlas.texture()
    });

    const sge::ShaderPipeline& particle_shader = Assets::GetShader(ShaderAsset::ParticleShader);

    sge::GraphicsPipelineConfig pipelineConfig;
    pipelineLayoutDesc.debugName = "ParticleRenderer Pipeline";
    pipelineConfig.vertexShader = particle_shader.vs;
    pipelineConfig.geometryShader = particle_shader.gs;
    pipelineConfig.pixelShader = particle_shader.ps;
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
                .srcAlpha = LLGL::BlendOp::Zero,
                .dstAlpha = LLGL::BlendOp::One,
                .alphaArithmetic = LLGL::BlendArithmetic::Max
            }
        }
    };

    m_pipeline = render_context->CreatePipelineState(pipelineConfig);

    LLGL::PipelineLayoutDescriptor compute_pipeline_layout_desc;
    compute_pipeline_layout_desc.heapBindings = sge::BindingLayout(
        LLGL::StageFlags::ComputeStage,
        {
            sge::BindingLayoutItem::ConstantBuffer(2, "GlobalUniformBuffer"),
            sge::BindingLayoutItem::StorageBuffer(5, "TransformBuffer"),
            sge::BindingLayoutItem::Buffer(6, "PositionBuffer"),
            sge::BindingLayoutItem::Buffer(7, "RotationBuffer"),
            sge::BindingLayoutItem::Buffer(8, "ScaleBuffer"),
        }
    );

    m_compute_pipeline_layout = render_context->CreatePipelineLayout(compute_pipeline_layout_desc);
    m_compute_resource_heap = render_context->CreateResourceHeap(m_compute_pipeline_layout, {
        m_renderer->GlobalUniformBuffer().Get(),
        m_transform_buffer.Get(),
        m_position_buffer.Get(),
        m_rotation_buffer.Get(),
        m_scale_buffer.Get()
    });

    sge::Ref<LLGL::Shader> computeShader = Assets::GetComputeShader(ComputeShaderAsset::ParticleComputeTransformShader);

    sge::ComputePipelineConfig compute_pipeline_desc;
    compute_pipeline_desc.pipelineLayout = m_compute_pipeline_layout.Get();
    compute_pipeline_desc.computeShader = computeShader.Get();

    m_compute_pipeline = render_context->CreateComputePipelineState(compute_pipeline_desc);
    if (const LLGL::Report* report = m_compute_pipeline->GetReport()) {
        if (report->HasErrors()) SGE_LOG_ERROR("{}", report->GetText());
    }

    m_is_metal = backend.IsMetal();

    reset();
}

void ParticleRenderer::draw_particle(const glm::vec2& position, const glm::quat& rotation, float scale, Particle::Type type, uint8_t variant, sge::Order) {
    ZoneScoped;

    const sge::Rect& rect = m_atlas.get_rect(get_particle_index(type, variant));

    *m_position_buffer_data_ptr = position;
    m_position_buffer_data_ptr++;

    *m_rotation_buffer_data_ptr = rotation;
    m_rotation_buffer_data_ptr++;

    *m_scale_buffer_data_ptr = scale;
    m_scale_buffer_data_ptr++;

    m_instance_buffer_data_ptr->uv = rect.min;
    m_instance_buffer_data_ptr->depth = 1.0f;
    m_instance_buffer_data_ptr->id = m_particle_id++;
    m_instance_buffer_data_ptr->is_world = false;
    m_instance_buffer_data_ptr++;

    m_particle_count++;
}

void ParticleRenderer::draw_particle_world(const glm::vec2& position, const glm::quat& rotation, float scale, Particle::Type type, uint8_t variant, sge::Order) {
    ZoneScoped;

    const sge::Rect& rect = m_atlas.get_rect(get_particle_index(type, variant));

    *m_position_buffer_data_ptr = position;
    m_position_buffer_data_ptr++;

    *m_rotation_buffer_data_ptr = rotation;
    m_rotation_buffer_data_ptr++;

    *m_scale_buffer_data_ptr = scale;
    m_scale_buffer_data_ptr++;

    m_instance_buffer_data_world_ptr->uv = rect.min;
    m_instance_buffer_data_world_ptr->depth = 1.0f;
    m_instance_buffer_data_world_ptr->id = m_particle_id++;
    m_instance_buffer_data_world_ptr->is_world = true;
    m_instance_buffer_data_world_ptr++;

    m_world_particle_count++;
}

void ParticleRenderer::compute() {
    if (m_particle_id == 0) return;

    ZoneScoped;

    const auto& commands = m_renderer->CommandBuffer();

    ptrdiff_t size = (uint8_t*) m_position_buffer_data_ptr - (uint8_t*) m_position_buffer_data.data();
    commands->UpdateBuffer(*m_position_buffer, 0, m_position_buffer_data.data(), size);

    size = (uint8_t*) m_rotation_buffer_data_ptr - (uint8_t*) m_rotation_buffer_data.data();
    commands->UpdateBuffer(*m_rotation_buffer, 0, m_rotation_buffer_data.data(), size);

    size = (uint8_t*) m_scale_buffer_data_ptr - (uint8_t*) m_scale_buffer_data.data();
    commands->UpdateBuffer(*m_scale_buffer, 0, m_scale_buffer_data.data(), size);

    commands->PushDebugGroup("CS ComputeTransform");
    {
        commands->SetPipelineState(*m_compute_pipeline);
        commands->SetResourceHeap(*m_compute_resource_heap);

        // particles count <= 1_000_000, 1_000_000 / 32 = 31250 < 65535
        const uint32_t x = (m_particle_id + 32 - 1) / 32;
        commands->Dispatch(x, 1, 1);
    }
    commands->PopDebugGroup();
}

void ParticleRenderer::prepare() {
    ZoneScoped;

    const auto& commands = m_renderer->CommandBuffer();

    if (m_particle_count > 0) {
        const ptrdiff_t size = (uint8_t*) m_instance_buffer_data_ptr - (uint8_t*) m_instance_buffer_data.data();
        commands->UpdateBuffer(*m_instance_buffer, 0, m_instance_buffer_data.data(), size);
    }

    if (m_world_particle_count > 0) {
        const ptrdiff_t size = (uint8_t*) m_instance_buffer_data_world_ptr - (uint8_t*) m_instance_buffer_data_world.data();
        commands->UpdateBuffer(*m_instance_buffer, m_particle_count * sizeof(ParticleInstance), m_instance_buffer_data_world.data(), size);
    }
}

void ParticleRenderer::render() {
    if (m_particle_count == 0) return;

    ZoneScoped;

    const auto& commands = m_renderer->CommandBuffer();

    commands->SetVertexBufferArray(*m_buffer_array);
    commands->SetPipelineState(m_renderer->GetRenderContext()->GetOrCreatePipeline(m_pipeline));

    commands->SetResourceHeap(*m_resource_heap);

    commands->DrawInstanced(4, 0, m_particle_count, 0);
}

void ParticleRenderer::render_world() {
    if (m_world_particle_count == 0) return;

    ZoneScoped;

    const auto& commands = m_renderer->CommandBuffer();

    commands->SetVertexBufferArray(*m_buffer_array);
    commands->SetPipelineState(m_renderer->GetRenderContext()->GetOrCreatePipeline(m_pipeline));

    commands->SetResourceHeap(*m_resource_heap);

    commands->DrawInstanced(4, 0, m_world_particle_count, m_particle_count);
}

void ParticleRenderer::reset() {
    m_instance_buffer_data_ptr = m_instance_buffer_data.data();
    m_instance_buffer_data_world_ptr = m_instance_buffer_data_world.data();
    m_rotation_buffer_data_ptr = m_rotation_buffer_data.data();
    m_position_buffer_data_ptr = m_position_buffer_data.data();
    m_scale_buffer_data_ptr = m_scale_buffer_data.data();
    m_particle_count = 0;
    m_world_particle_count = 0;
    m_particle_id = 0;
}

ParticleRenderer::~ParticleRenderer() {
    m_renderer->GetRenderContext()->DeletePipeline(m_pipeline);
}
