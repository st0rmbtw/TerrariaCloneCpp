#include "particle_renderer.hpp"

#include "LLGL/Format.h"
#include "LLGL/ShaderFlags.h"
#include "../assets.hpp"
#include "renderer.hpp"

#include "../log.hpp"
#include "utils.hpp"

using Constants::PARTICLE_SIZE;

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

    m_atlas = Assets::GetTextureAtlas(TextureAsset::Particles);

    m_instance_buffer_data = new ParticleInstance[MAX_PARTICLES_COUNT];
    m_instance_buffer_data_ptr = m_instance_buffer_data;

    m_position_buffer_data = new glm::vec2[MAX_PARTICLES_COUNT];
    m_position_buffer_data_ptr = m_position_buffer_data;

    m_rotation_buffer_data = new glm::quat[MAX_PARTICLES_COUNT];
    m_rotation_buffer_data_ptr = m_rotation_buffer_data;

    m_scale_buffer_data = new float[MAX_PARTICLES_COUNT];
    m_scale_buffer_data_ptr = m_scale_buffer_data;

    const ParticleVertex vertices[] = {
        ParticleVertex(0.0, 0.0, PARTICLE_SIZE / glm::vec2(m_atlas.texture().size), glm::vec2(m_atlas.texture().size)), // 0
        ParticleVertex(0.0, 1.0, PARTICLE_SIZE / glm::vec2(m_atlas.texture().size), glm::vec2(m_atlas.texture().size)), // 1
        ParticleVertex(1.0, 0.0, PARTICLE_SIZE / glm::vec2(m_atlas.texture().size), glm::vec2(m_atlas.texture().size)), // 2
        ParticleVertex(1.0, 1.0, PARTICLE_SIZE / glm::vec2(m_atlas.texture().size), glm::vec2(m_atlas.texture().size)), // 2
    };

    m_vertex_buffer = CreateVertexBufferInit(sizeof(vertices), vertices, Assets::GetVertexFormat(VertexFormatAsset::ParticleVertex), "ParticleRenderer VertexBuffer");
    m_instance_buffer = CreateVertexBuffer(MAX_PARTICLES_COUNT * sizeof(ParticleInstance), Assets::GetVertexFormat(VertexFormatAsset::ParticleInstance), "ParticleRenderer InstanceBuffer");

    LLGL::Buffer* buffers[] = { m_vertex_buffer, m_instance_buffer };
    m_buffer_array = context->CreateBufferArray(2, buffers);

    {
        long bindFlags = LLGL::BindFlags::Storage;
        LLGL::BufferDescriptor bufferDesc;
        bufferDesc.size         = sizeof(glm::mat4) * MAX_PARTICLES_COUNT;
        bufferDesc.bindFlags    = bindFlags;
        bufferDesc.format       = LLGL::Format::RGBA32Float;

        m_transform_buffer = context->CreateBuffer(bufferDesc);
    }

    {
        long bindFlags = LLGL::BindFlags::Sampled;
        LLGL::BufferDescriptor bufferDesc;
        bufferDesc.size         = sizeof(glm::vec2) * MAX_PARTICLES_COUNT;
        bufferDesc.bindFlags    = bindFlags;
        bufferDesc.format       = LLGL::Format::RG32Float;

        m_position_buffer = context->CreateBuffer(bufferDesc);
    }

    {
        long bindFlags = LLGL::BindFlags::Sampled;
        LLGL::BufferDescriptor bufferDesc;
        bufferDesc.size         = sizeof(glm::vec4) * MAX_PARTICLES_COUNT;
        bufferDesc.bindFlags    = bindFlags;
        bufferDesc.format       = LLGL::Format::RGBA32Float;

        m_rotation_buffer = context->CreateBuffer(bufferDesc);
    }

    {
        long bindFlags = LLGL::BindFlags::Sampled;
        LLGL::BufferDescriptor bufferDesc;
        bufferDesc.size         = sizeof(float) * MAX_PARTICLES_COUNT;
        bufferDesc.bindFlags    = bindFlags;
        bufferDesc.format       = LLGL::Format::R32Float;

        m_scale_buffer = context->CreateBuffer(bufferDesc);
    } 

    const uint32_t samplerBinding = Renderer::Backend().IsOpenGL() ? 2 : 3;

    LLGL::PipelineLayoutDescriptor pipelineLayoutDesc;
    pipelineLayoutDesc.bindings = {
        LLGL::BindingDescriptor(
            "GlobalUniformBuffer",
            LLGL::ResourceType::Buffer,
            LLGL::BindFlags::ConstantBuffer,
            LLGL::StageFlags::ComputeStage,
            LLGL::BindingSlot(1)
        ),
        LLGL::BindingDescriptor("u_texture", LLGL::ResourceType::Texture, LLGL::BindFlags::Sampled, LLGL::StageFlags::FragmentStage, LLGL::BindingSlot(2)),
        LLGL::BindingDescriptor("u_sampler", LLGL::ResourceType::Sampler, 0, LLGL::StageFlags::FragmentStage, LLGL::BindingSlot(samplerBinding)),
        LLGL::BindingDescriptor(
            "TransformBuffer",
            LLGL::ResourceType::Buffer,
            LLGL::BindFlags::Storage,
            LLGL::StageFlags::VertexStage | LLGL::StageFlags::ComputeStage,
            LLGL::BindingSlot(4)
        ),
    };

    LLGL::PipelineLayout* pipelineLayout = context->CreatePipelineLayout(pipelineLayoutDesc);

    const ShaderPipeline& particle_shader = Assets::GetShader(ShaderAsset::ParticleShader);

    LLGL::GraphicsPipelineDescriptor pipelineDesc;
    pipelineLayoutDesc.debugName = "ParticleRenderer Pipeline";
    pipelineDesc.vertexShader = particle_shader.vs;
    pipelineDesc.geometryShader = particle_shader.gs;
    pipelineDesc.fragmentShader = particle_shader.ps;
    pipelineDesc.pipelineLayout = pipelineLayout;
    pipelineDesc.indexFormat = LLGL::Format::Undefined;
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
                .dstAlpha = LLGL::BlendOp::One
            }
        }
    };

    m_pipeline = context->CreatePipelineState(pipelineDesc);
    if (const LLGL::Report* report = m_pipeline->GetReport()) {
        if (report->HasErrors()) LOG_ERROR("%s", report->GetText());
    }

    LLGL::PipelineLayoutDescriptor compute_pipeline_layout_desc;
    compute_pipeline_layout_desc.bindings = {
        LLGL::BindingDescriptor(
            "TransformBuffer",
            LLGL::ResourceType::Buffer,
            LLGL::BindFlags::Storage,
            LLGL::StageFlags::ComputeStage,
            LLGL::BindingSlot(4)
        ),
        LLGL::BindingDescriptor(
            "PositionBuffer",
            LLGL::ResourceType::Buffer,
            LLGL::BindFlags::Sampled,
            LLGL::StageFlags::ComputeStage,
            LLGL::BindingSlot(5)
        ),
        LLGL::BindingDescriptor(
            "RotationBuffer",
            LLGL::ResourceType::Buffer,
            LLGL::BindFlags::Sampled,
            LLGL::StageFlags::ComputeStage,
            LLGL::BindingSlot(6)
        ),
        LLGL::BindingDescriptor(
            "ScaleBuffer",
            LLGL::ResourceType::Buffer,
            LLGL::BindFlags::Sampled,
            LLGL::StageFlags::ComputeStage,
            LLGL::BindingSlot(7)
        ),
    };

    LLGL::PipelineLayout* compute_pipeline_layout = context->CreatePipelineLayout(compute_pipeline_layout_desc);

    LLGL::Shader* computeShader = Assets::GetComputeShader(ComputeShaderAsset::ParticleComputeTransformShader);

    LLGL::ComputePipelineDescriptor compute_pipeline_desc;
    compute_pipeline_desc.pipelineLayout = compute_pipeline_layout;
    compute_pipeline_desc.computeShader = computeShader;

    m_compute_pipeline = context->CreatePipelineState(compute_pipeline_desc);
    if (const LLGL::Report* report = m_compute_pipeline->GetReport()) {
        if (report->HasErrors()) LOG_ERROR("%s", report->GetText());
    }
}

void ParticleRenderer::draw_particle(const glm::vec2& position, const glm::quat& rotation, float scale, Particle::Type type, uint8_t variant, int depth) {
    const math::Rect& rect = m_atlas.get_rect(get_particle_index(type, variant));

    *m_position_buffer_data_ptr = position;
    m_position_buffer_data_ptr++;

    *m_rotation_buffer_data_ptr = rotation;
    m_rotation_buffer_data_ptr++;

    *m_scale_buffer_data_ptr = scale;
    m_scale_buffer_data_ptr++;

    m_instance_buffer_data_ptr->uv = rect.min;
    m_instance_buffer_data_ptr->depth = static_cast<float>(depth);
    m_instance_buffer_data_ptr++;

    m_particle_count++;
}

void ParticleRenderer::render() {
    if (m_particle_count == 0) return;

    auto* const commands = Renderer::CommandBuffer();

    ptrdiff_t size = (uint8_t*) m_instance_buffer_data_ptr - (uint8_t*) m_instance_buffer_data;
    if (size <= (1 << 16)) {
        commands->UpdateBuffer(*m_instance_buffer, 0, m_instance_buffer_data, size);
    } else {
        Renderer::Context()->WriteBuffer(*m_instance_buffer, 0, m_instance_buffer_data, size);
    }

    size = (uint8_t*) m_position_buffer_data_ptr - (uint8_t*) m_position_buffer_data;
    if (size <= (1 << 16)) {
        commands->UpdateBuffer(*m_position_buffer, 0, m_position_buffer_data, size);
    } else {
        Renderer::Context()->WriteBuffer(*m_position_buffer, 0, m_position_buffer_data, size);
    }

    size = (uint8_t*) m_rotation_buffer_data_ptr - (uint8_t*) m_rotation_buffer_data;
    if (size <= (1 << 16)) {
        commands->UpdateBuffer(*m_rotation_buffer, 0, m_rotation_buffer_data, size);
    } else {
        Renderer::Context()->WriteBuffer(*m_rotation_buffer, 0, m_rotation_buffer_data, size);
    }

    size = (uint8_t*) m_scale_buffer_data_ptr - (uint8_t*) m_scale_buffer_data;
    if (size <= (1 << 16)) {
        commands->UpdateBuffer(*m_scale_buffer, 0, m_scale_buffer_data, size);
    } else {
        Renderer::Context()->WriteBuffer(*m_scale_buffer, 0, m_scale_buffer_data, size);
    }

    commands->PushDebugGroup("CS ComputeTransform");
    {
        commands->SetPipelineState(*m_compute_pipeline);

        commands->SetResource(0, *m_transform_buffer);
        commands->SetResource(1, *m_position_buffer);
        commands->SetResource(2, *m_rotation_buffer);
        commands->SetResource(3, *m_scale_buffer);

        const uint32_t y = glm::max(1u, m_particle_count / 100);

        commands->Dispatch(10, y, 1);
    }
    commands->PopDebugGroup();
    
    commands->SetVertexBufferArray(*m_buffer_array);

    commands->BeginRenderPass(*Renderer::SwapChain(), Renderer::DefaultRenderPass());
        commands->SetPipelineState(*m_pipeline);

        const Texture& t = m_atlas.texture();

        commands->SetResource(0, *Renderer::GlobalUniformBuffer());
        commands->SetResource(1, *t.texture);
        commands->SetResource(2, Assets::GetSampler(t.sampler));
        commands->SetResource(3, *m_transform_buffer);

        commands->DrawInstanced(4, 0, m_particle_count, 0);
    commands->EndRenderPass();

    m_instance_buffer_data_ptr = m_instance_buffer_data;
    m_rotation_buffer_data_ptr = m_rotation_buffer_data;
    m_position_buffer_data_ptr = m_position_buffer_data;
    m_scale_buffer_data_ptr = m_scale_buffer_data;
    
    m_particle_count = 0;
}

void ParticleRenderer::terminate() {
    const auto& context = Renderer::Context();

    if (m_vertex_buffer) context->Release(*m_vertex_buffer);
    if (m_pipeline) context->Release(*m_pipeline);
}