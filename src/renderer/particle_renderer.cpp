#include "particle_renderer.hpp"

#include <LLGL/Format.h>
#include <LLGL/PipelineLayoutFlags.h>
#include <LLGL/ShaderFlags.h>

#include <SGE/engine.hpp>
#include <SGE/log.hpp>
#include <SGE/renderer/macros.hpp>
#include <SGE/types/binding_layout.hpp>

#include <tracy/Tracy.hpp>

#include "types.hpp"

#include "../assets.hpp"

using Constants::PARTICLE_SIZE;
using Constants::MAX_PARTICLES_COUNT;

static inline constexpr size_t get_particle_index(Particle::Type type, uint8_t variant) {
    SGE_ASSERT(variant <= 2, "Variant must be in range from 0 to 3");

    const size_t index = static_cast<uint8_t>(type);
    const size_t y = index / PARTICLES_ATLAS_COLUMNS;
    const size_t x = index % PARTICLES_ATLAS_COLUMNS;
    return (y * 3 + variant) * PARTICLES_ATLAS_COLUMNS + x;
}

void ParticleRenderer::init() {
    ZoneScopedN("ParticleRenderer::init");

    m_renderer = &sge::Engine::Renderer();

    const sge::RenderBackend backend = m_renderer->Backend();
    const auto& context = m_renderer->Context();
    const auto* swap_chain = m_renderer->SwapChain();

    m_atlas = Assets::GetTextureAtlas(TextureAsset::Particles);

    m_instance_buffer_data = new ParticleInstance[MAX_PARTICLES_COUNT];
    m_instance_buffer_data_world = new ParticleInstance[MAX_PARTICLES_COUNT];
    
    m_position_buffer_data = new glm::vec2[MAX_PARTICLES_COUNT];
    m_rotation_buffer_data = new glm::quat[MAX_PARTICLES_COUNT];
    m_scale_buffer_data = new float[MAX_PARTICLES_COUNT];

    const ParticleVertex vertices[] = {
        ParticleVertex(0.0, 0.0, PARTICLE_SIZE / glm::vec2(m_atlas.texture().size()), glm::vec2(m_atlas.texture().size())),
        ParticleVertex(0.0, 1.0, PARTICLE_SIZE / glm::vec2(m_atlas.texture().size()), glm::vec2(m_atlas.texture().size())),
        ParticleVertex(1.0, 0.0, PARTICLE_SIZE / glm::vec2(m_atlas.texture().size()), glm::vec2(m_atlas.texture().size())),
        ParticleVertex(1.0, 1.0, PARTICLE_SIZE / glm::vec2(m_atlas.texture().size()), glm::vec2(m_atlas.texture().size())),
    };

    m_vertex_buffer = m_renderer->CreateVertexBufferInit(sizeof(vertices), vertices, Assets::GetVertexFormat(VertexFormatAsset::ParticleVertex), "ParticleRenderer VertexBuffer");
    m_instance_buffer = m_renderer->CreateVertexBuffer(MAX_PARTICLES_COUNT * sizeof(ParticleInstance), Assets::GetVertexFormat(VertexFormatAsset::ParticleInstance), "ParticleRenderer InstanceBuffer");

    LLGL::Buffer* buffers[] = { m_vertex_buffer, m_instance_buffer };
    m_buffer_array = context->CreateBufferArray(2, buffers);

    {
        long bindFlags = LLGL::BindFlags::Storage;
        LLGL::BufferDescriptor bufferDesc;
        bufferDesc.debugName = "Particle TransformBuffer";
        bufferDesc.size         = sizeof(glm::mat4) * MAX_PARTICLES_COUNT;
        bufferDesc.bindFlags    = bindFlags;
        bufferDesc.format       = LLGL::Format::RGBA32Float;

        m_transform_buffer = context->CreateBuffer(bufferDesc);
    }

    {
        long bindFlags = LLGL::BindFlags::Sampled;
        LLGL::BufferDescriptor bufferDesc;
        bufferDesc.debugName = "Particle PositionBuffer";
        bufferDesc.size         = sizeof(glm::vec2) * MAX_PARTICLES_COUNT;
        bufferDesc.bindFlags    = bindFlags;
        bufferDesc.format       = LLGL::Format::RG32Float;

        m_position_buffer = context->CreateBuffer(bufferDesc);
    }

    {
        long bindFlags = LLGL::BindFlags::Sampled;
        LLGL::BufferDescriptor bufferDesc;
        bufferDesc.debugName = "Particle RotationBuffer";
        bufferDesc.size         = sizeof(glm::vec4) * MAX_PARTICLES_COUNT;
        bufferDesc.bindFlags    = bindFlags;
        bufferDesc.format       = LLGL::Format::RGBA32Float;

        m_rotation_buffer = context->CreateBuffer(bufferDesc);
    }

    {
        long bindFlags = LLGL::BindFlags::Sampled;
        LLGL::BufferDescriptor bufferDesc;
        bufferDesc.debugName = "Particle ScaleBuffer";
        bufferDesc.size         = sizeof(float) * MAX_PARTICLES_COUNT;
        bufferDesc.bindFlags    = bindFlags;
        bufferDesc.format       = LLGL::Format::R32Float;

        m_scale_buffer = context->CreateBuffer(bufferDesc);
    }

    LLGL::PipelineLayoutDescriptor pipelineLayoutDesc;
    pipelineLayoutDesc.heapBindings = sge::BindingLayout(
        LLGL::StageFlags::VertexStage | LLGL::StageFlags::FragmentStage,
        {
            sge::BindingLayoutItem::ConstantBuffer(2, "GlobalUniformBuffer"),
            sge::BindingLayoutItem::StorageBuffer(5, "TransformBuffer"),
            sge::BindingLayoutItem::Texture(3, "u_texture"),
        }
    );
    pipelineLayoutDesc.staticSamplers = {
        LLGL::StaticSamplerDescriptor("u_sampler", LLGL::StageFlags::FragmentStage, LLGL::BindingSlot(4), m_atlas.texture().sampler().descriptor())
    };
    pipelineLayoutDesc.combinedTextureSamplers = {
        LLGL::CombinedTextureSamplerDescriptor{ "u_texture", "u_texture", "u_sampler", 3 }
    };

    LLGL::PipelineLayout* pipelineLayout = context->CreatePipelineLayout(pipelineLayoutDesc);
    {
        const LLGL::ResourceViewDescriptor resource_views[] = {
            m_renderer->GlobalUniformBuffer(), m_transform_buffer, m_atlas.texture()
        };

        m_resource_heap = context->CreateResourceHeap(pipelineLayout, resource_views);
    }

    const sge::ShaderPipeline& particle_shader = Assets::GetShader(ShaderAsset::ParticleShader);

    LLGL::GraphicsPipelineDescriptor pipelineDesc;
    pipelineLayoutDesc.debugName = "ParticleRenderer Pipeline";
    pipelineDesc.vertexShader = particle_shader.vs;
    pipelineDesc.geometryShader = particle_shader.gs;
    pipelineDesc.fragmentShader = particle_shader.ps;
    pipelineDesc.pipelineLayout = pipelineLayout;
    pipelineDesc.indexFormat = LLGL::Format::R16UInt;
    pipelineDesc.primitiveTopology = LLGL::PrimitiveTopology::TriangleStrip;
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
                .dstAlpha = LLGL::BlendOp::One,
                .alphaArithmetic = LLGL::BlendArithmetic::Max
            }
        }
    };

    m_pipeline = context->CreatePipelineState(pipelineDesc);
    if (const LLGL::Report* report = m_pipeline->GetReport()) {
        if (report->HasErrors()) SGE_LOG_ERROR("%s", report->GetText());
    }

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

    LLGL::PipelineLayout* compute_pipeline_layout = context->CreatePipelineLayout(compute_pipeline_layout_desc);

    const LLGL::ResourceViewDescriptor resource_views[] = {
        m_renderer->GlobalUniformBuffer(), m_transform_buffer, m_position_buffer, m_rotation_buffer, m_scale_buffer
    };

    m_compute_resource_heap = context->CreateResourceHeap(compute_pipeline_layout, resource_views);

    LLGL::Shader* computeShader = Assets::GetComputeShader(ComputeShaderAsset::ParticleComputeTransformShader);

    LLGL::ComputePipelineDescriptor compute_pipeline_desc;
    compute_pipeline_desc.pipelineLayout = compute_pipeline_layout;
    compute_pipeline_desc.computeShader = computeShader;

    m_compute_pipeline = context->CreatePipelineState(compute_pipeline_desc);
    if (const LLGL::Report* report = m_compute_pipeline->GetReport()) {
        if (report->HasErrors()) SGE_LOG_ERROR("%s", report->GetText());
    }

    m_is_metal = backend.IsMetal();

    reset();
}

void ParticleRenderer::draw_particle(const glm::vec2& position, const glm::quat& rotation, float scale, Particle::Type type, uint8_t variant, sge::Order) {
    ZoneScopedN("ParticleRenderer::draw_particle");

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
    ZoneScopedN("ParticleRenderer::draw_particle");

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

    ZoneScopedN("ParticleRenderer::compute");

    const auto& context = m_renderer->Context();
    auto* const commands = m_renderer->CommandBuffer();

    ptrdiff_t size = (uint8_t*) m_position_buffer_data_ptr - (uint8_t*) m_position_buffer_data;
    if (size < (1 << 16)) {
        commands->UpdateBuffer(*m_position_buffer, 0, m_position_buffer_data, size);
    } else {
        context->WriteBuffer(*m_position_buffer, 0, m_position_buffer_data, size);
    }

    size = (uint8_t*) m_rotation_buffer_data_ptr - (uint8_t*) m_rotation_buffer_data;
    if (size < (1 << 16)) {
        commands->UpdateBuffer(*m_rotation_buffer, 0, m_rotation_buffer_data, size);
    } else {
        context->WriteBuffer(*m_rotation_buffer, 0, m_rotation_buffer_data, size);
    }

    size = (uint8_t*) m_scale_buffer_data_ptr - (uint8_t*) m_scale_buffer_data;
    if (size < (1 << 16)) {
        commands->UpdateBuffer(*m_scale_buffer, 0, m_scale_buffer_data, size);
    } else {
        context->WriteBuffer(*m_scale_buffer, 0, m_scale_buffer_data, size);
    }

    commands->PushDebugGroup("CS ComputeTransform");
    {
        commands->SetPipelineState(*m_compute_pipeline);
        commands->SetResourceHeap(*m_compute_resource_heap);

        const size_t particle_count = m_particle_id;

        if (m_is_metal) {
            uint32_t y = (particle_count + 512 - 1) / 512;
            commands->Dispatch(512, y, 1);
        } else {
            // particles_count < 1_000_000, 1_000_000 / 64 = 15625 < 65535
            const uint32_t x = (particle_count + 64 - 1) / 64;
            commands->Dispatch(x, 1, 1);
        }
    }
    commands->PopDebugGroup();
}

void ParticleRenderer::prepare() {
    ZoneScopedN("ParticleRenderer::prepare");

    const auto& context = m_renderer->Context();
    auto* const commands = m_renderer->CommandBuffer();

    if (m_particle_count > 0) {
        const ptrdiff_t size = (uint8_t*) m_instance_buffer_data_ptr - (uint8_t*) m_instance_buffer_data;
        if (size < (1 << 16)) {
            commands->UpdateBuffer(*m_instance_buffer, 0, m_instance_buffer_data, size);
        } else {
            context->WriteBuffer(*m_instance_buffer, 0, m_instance_buffer_data, size);
        }
    }

    if (m_world_particle_count > 0) {
        const ptrdiff_t size = (uint8_t*) m_instance_buffer_data_world_ptr - (uint8_t*) m_instance_buffer_data_world;
        if (size < (1 << 16)) {
            commands->UpdateBuffer(*m_instance_buffer, m_particle_count * sizeof(ParticleInstance), m_instance_buffer_data_world, size);
        } else {
            context->WriteBuffer(*m_instance_buffer, m_particle_count * sizeof(ParticleInstance), m_instance_buffer_data_world, size);
        }
    }
}

void ParticleRenderer::render() {
    if (m_particle_count == 0) return;
    
    ZoneScopedN("ParticleRenderer::render");

    auto* const commands = m_renderer->CommandBuffer();

    commands->SetVertexBufferArray(*m_buffer_array);
    commands->SetPipelineState(*m_pipeline);

    commands->SetResourceHeap(*m_resource_heap);

    commands->DrawInstanced(4, 0, m_particle_count, 0);
}

void ParticleRenderer::render_world() {
    if (m_world_particle_count == 0) return;
    
    ZoneScopedN("ParticleRenderer::render_world");

    auto* const commands = m_renderer->CommandBuffer();

    commands->SetVertexBufferArray(*m_buffer_array);
    commands->SetPipelineState(*m_pipeline);

    commands->SetResourceHeap(*m_resource_heap);

    commands->DrawInstanced(4, 0, m_world_particle_count, m_particle_count);
}

void ParticleRenderer::reset() {
    m_instance_buffer_data_ptr = m_instance_buffer_data;
    m_instance_buffer_data_world_ptr = m_instance_buffer_data_world;
    m_rotation_buffer_data_ptr = m_rotation_buffer_data;
    m_position_buffer_data_ptr = m_position_buffer_data;
    m_scale_buffer_data_ptr = m_scale_buffer_data;
    m_particle_count = 0;
    m_world_particle_count = 0;
    m_particle_id = 0;
}

void ParticleRenderer::terminate() {
    const auto& context = m_renderer->Context();

    SGE_RESOURCE_RELEASE(m_buffer_array);
    SGE_RESOURCE_RELEASE(m_instance_buffer);
    SGE_RESOURCE_RELEASE(m_vertex_buffer);

    SGE_RESOURCE_RELEASE(m_position_buffer);
    SGE_RESOURCE_RELEASE(m_rotation_buffer);
    SGE_RESOURCE_RELEASE(m_scale_buffer);

    SGE_RESOURCE_RELEASE(m_transform_buffer);

    SGE_RESOURCE_RELEASE(m_pipeline);
    SGE_RESOURCE_RELEASE(m_compute_pipeline);

    delete[] m_instance_buffer_data;
    delete[] m_instance_buffer_data_world;
    delete[] m_position_buffer_data;
    delete[] m_rotation_buffer_data;
    delete[] m_scale_buffer_data;
}