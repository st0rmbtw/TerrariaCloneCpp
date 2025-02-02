#include "renderer.hpp"

#include <fstream>
#include <sstream>
#include <tracy/Tracy.hpp>

#include "../log.hpp"
#include "../../utils.hpp"

#include "batch.hpp"
#include "macros.hpp"

using namespace batch_internal;

bool Renderer::InitEngine(RenderBackend backend) {
    ZoneScopedN("Renderer::InitEngine");

    LLGL::Report report;

    LLGL::RenderSystemDescriptor rendererDesc;
    rendererDesc.moduleName = backend.ToString();

    if (backend.IsOpenGL()) {
        LLGL::RendererConfigurationOpenGL* config = new LLGL::RendererConfigurationOpenGL();
        config->majorVersion = 4;
        config->minorVersion = 3;
        config->contextProfile = LLGL::OpenGLContextProfile::CoreProfile;

        rendererDesc.rendererConfig = config;
        rendererDesc.rendererConfigSize = sizeof(LLGL::RendererConfigurationOpenGL);
    }

#if DEBUG
    m_debugger = new LLGL::RenderingDebugger();
    rendererDesc.flags    = LLGL::RenderSystemFlags::DebugDevice;
    rendererDesc.debugger = m_debugger;
#endif

    m_context = LLGL::RenderSystem::Load(rendererDesc, &report);
    m_backend = backend;

    if (backend.IsOpenGL()) {
        delete (LLGL::OpenGLContextProfile*) rendererDesc.rendererConfig;
    }

    if (report.HasErrors()) {
        LOG_ERROR("%s", report.GetText());
        return false;
    }

    const auto& info = m_context->GetRendererInfo();

    LOG_INFO("Renderer:             %s", info.rendererName.c_str());
    LOG_INFO("Device:               %s", info.deviceName.c_str());
    LOG_INFO("Vendor:               %s", info.vendorName.c_str());
    LOG_INFO("Shading Language:     %s", info.shadingLanguageName.c_str());

    LOG_INFO("Extensions:");
    for (const auto& extension : info.extensionNames) {
        LOG_INFO("  %s", extension.c_str());
    }

    return true;
}

bool Renderer::Init(GLFWwindow* window, const LLGL::Extent2D& resolution, bool vsync, bool fullscreen) {
    ZoneScopedN("Renderer::Init");

    const LLGL::RenderSystemPtr& context = m_context;

    m_surface = std::make_shared<CustomSurface>(window, resolution);

    LLGL::SwapChainDescriptor swapChainDesc;
    swapChainDesc.resolution = resolution;
    swapChainDesc.fullscreen = fullscreen;

    m_swap_chain = context->CreateSwapChain(swapChainDesc, m_surface);
    m_swap_chain->SetVsyncInterval(vsync ? 1 : 0);

    LLGL::CommandBufferDescriptor command_buffer_desc;
    command_buffer_desc.numNativeBuffers = 3;

    m_command_buffer = context->CreateCommandBuffer(command_buffer_desc);
    m_command_queue = context->GetCommandQueue();

    m_constant_buffer = CreateConstantBuffer(sizeof(ProjectionsUniform), "ConstantBuffer");

    InitSpriteBatchPipeline();
    InitNinepatchBatchPipeline();
    InitGlyphBatchPipeline();

    return true;
}

void Renderer::Begin(const Camera& camera) {
    ZoneScopedN("Renderer::Begin");

    auto projections_uniform = ProjectionsUniform {
        .screen_projection_matrix = camera.get_screen_projection_matrix(),
        .view_projection_matrix = camera.get_view_projection_matrix(),
        .nonscale_view_projection_matrix = camera.get_nonscale_view_projection_matrix(),
        .nonscale_projection_matrix = camera.get_nonscale_projection_matrix(),
        .transform_matrix = camera.get_transform_matrix(),
        .inv_view_proj_matrix = camera.get_inv_view_projection_matrix(),
        .camera_position = camera.position(),
        .window_size = camera.viewport()
    };

    m_command_buffer->Begin();
    m_command_buffer->SetViewport(m_swap_chain->GetResolution());
    m_command_buffer->UpdateBuffer(*m_constant_buffer, 0, &projections_uniform, sizeof(projections_uniform));
}

void Renderer::End() {
    ZoneScopedN("Renderer::End");

    m_command_buffer->End();
    m_command_queue->Submit(*m_command_buffer);

    m_swap_chain->Present();
}

void Renderer::ApplyBatchDrawCommands(Batch& batch) {
    ZoneScopedN("Renderer::ApplyBatchDrawCommands");

    auto* const commands = m_command_buffer;

    int prev_flush_data_type = -1;
    int prev_texture_id = -1;

    LLGL::PipelineState& sprite_pipeline = batch.depth_enabled()
        ? *m_sprite_batch_data.pipeline_depth
        : *m_sprite_batch_data.pipeline;

    for (const FlushData& flush_data : batch.flush_queue()) {
        if (prev_flush_data_type != static_cast<int>(flush_data.type)) {
            switch (flush_data.type) {
            case FlushDataType::Sprite:
                commands->SetVertexBufferArray(*m_sprite_batch_data.buffer_array);
                commands->SetPipelineState(sprite_pipeline);
            break;

            case FlushDataType::Glyph:
                commands->SetVertexBufferArray(*m_glyph_batch_data.buffer_array);
                commands->SetPipelineState(*m_glyph_batch_data.pipeline);
            break;

            case FlushDataType::NinePatch:
                commands->SetVertexBufferArray(*m_ninepatch_batch_data.buffer_array);
                commands->SetPipelineState(*m_ninepatch_batch_data.pipeline);
            break;
            }

            commands->SetResource(0, *m_constant_buffer);
        }

        if (prev_texture_id != flush_data.texture.id()) {
            commands->SetResource(1, flush_data.texture);
            commands->SetResource(2, Assets::GetSampler(flush_data.texture));
        }
        
        commands->DrawInstanced(4, 0, flush_data.count, flush_data.offset);

        prev_flush_data_type = static_cast<int>(flush_data.type);
        prev_texture_id = flush_data.texture.id();
    }
}

void Renderer::RenderBatch(Batch& batch) {
    batch.SortDrawCommands();

    const size_t sprite_size = batch.sprite_buffer_size();
    if (sprite_size > 0) {
        UpdateBuffer(m_sprite_batch_data.instance_buffer, batch.sprite_buffer(), sprite_size);
    }

    const size_t glyph_size = batch.glyph_buffer_size();
    if (glyph_size > 0) {
        UpdateBuffer(m_glyph_batch_data.instance_buffer, batch.glyph_buffer(), glyph_size);
    }

    const size_t ninepatch_size = batch.glyph_buffer_size();
    if (ninepatch_size > 0) {
        UpdateBuffer(m_ninepatch_batch_data.instance_buffer, batch.glyph_buffer(), ninepatch_size);
    }

    ApplyBatchDrawCommands(batch);
    batch.Reset();
}

Texture Renderer::CreateTexture(LLGL::TextureType type, LLGL::ImageFormat image_format, LLGL::DataType data_type, uint32_t width, uint32_t height, uint32_t layers, int sampler, const void* data, bool generate_mip_maps) {
    ZoneScopedN("Renderer::CreateTexture");

    LLGL::TextureDescriptor texture_desc;
    texture_desc.type = type;
    texture_desc.extent = LLGL::Extent3D(width, height, 1);
    texture_desc.arrayLayers = layers;
    texture_desc.bindFlags = LLGL::BindFlags::Sampled | LLGL::BindFlags::ColorAttachment;
    texture_desc.cpuAccessFlags = 0;
    texture_desc.miscFlags = LLGL::MiscFlags::GenerateMips * generate_mip_maps;
    texture_desc.mipLevels = generate_mip_maps ? 0 : 1;

    uint32_t components = LLGL::ImageFormatSize(image_format);

    LLGL::ImageView image_view;
    image_view.format = image_format;
    image_view.dataType = data_type;
    image_view.data = data;
    image_view.dataSize = width * height * layers * components;

    uint32_t id = m_texture_index++;

    return Texture(id, sampler, glm::uvec2(width, height), m_context->CreateTexture(texture_desc, &image_view));
}

LLGL::Shader* Renderer::LoadShader(const ShaderPath& shader_path, const std::vector<ShaderDef>& shader_defs, const std::vector<LLGL::VertexAttribute>& vertex_attributes) {
    ZoneScopedN("Renderer::LoadShader");

    const RenderBackend backend = m_backend;
    const ShaderType shader_type = shader_path.shader_type;

    const std::string path = backend.AssetFolder() + shader_path.name + shader_type.FileExtension(backend);

    if (!FileExists(path.c_str())) {
        LOG_ERROR("Failed to find shader '%s'", path.c_str());
        return nullptr;
    }

    std::string shader_source;

    if (!backend.IsVulkan()) {
        std::ifstream shader_file;
        shader_file.open(path);

        std::stringstream shader_source_str;
        shader_source_str << shader_file.rdbuf();

        shader_source = shader_source_str.str();

        for (const ShaderDef& shader_def : shader_defs) {
            size_t pos;
            while ((pos = shader_source.find(shader_def.name)) != std::string::npos) {
                shader_source.replace(pos, shader_def.name.length(), shader_def.value);
            }
        }

        shader_file.close();
    }

    LLGL::ShaderDescriptor shader_desc;
    shader_desc.type = shader_type.ToLLGLType();
    shader_desc.sourceType = LLGL::ShaderSourceType::CodeString;

    if (shader_type.IsVertex()) {
        shader_desc.vertex.inputAttribs = vertex_attributes;
    }

    if (backend.IsOpenGL() && shader_type.IsFragment()) {
        shader_desc.fragment.outputAttribs = {
            { "frag_color", LLGL::Format::RGBA8UNorm, 0, LLGL::SystemValue::Color }
        };
    }

    if (backend.IsVulkan()) {
        shader_desc.source = path.c_str();
        shader_desc.sourceType = LLGL::ShaderSourceType::BinaryFile;
    } else {
        shader_desc.entryPoint = shader_type.IsCompute() ? shader_path.func_name.c_str() : shader_type.EntryPoint(backend);
        shader_desc.source = shader_source.c_str();
        shader_desc.sourceSize = shader_source.length();
        shader_desc.profile = shader_type.Profile(backend);
    }

#if DEBUG
    shader_desc.flags |= LLGL::ShaderCompileFlags::NoOptimization;
#else
    shader_desc.flags |= LLGL::ShaderCompileFlags::OptimizationLevel3;
#endif

    LLGL::Shader* shader = m_context->CreateShader(shader_desc);
    if (const LLGL::Report* report = shader->GetReport()) {
        if (*report->GetText() != '\0') {
            if (report->HasErrors()) {
                LOG_ERROR("Failed to create a shader. File: %s\nError: %s", path.c_str(), report->GetText());
                return nullptr;
            }
            
            LOG_INFO("%s", report->GetText());
        }
    }

    return shader;
}

#if DEBUG
void Renderer::PrintDebugInfo() {
    LLGL::FrameProfile profile;
    m_debugger->FlushProfile(&profile);

    LOG_DEBUG("Draw commands count: %u", profile.commandBufferRecord.drawCommands);
}
#endif

void Renderer::Terminate() {
    const auto& context = m_context;

    RESOURCE_RELEASE(m_sprite_batch_data.vertex_buffer)
    RESOURCE_RELEASE(m_sprite_batch_data.instance_buffer)
    RESOURCE_RELEASE(m_sprite_batch_data.buffer_array)
    RESOURCE_RELEASE(m_sprite_batch_data.pipeline)

    RESOURCE_RELEASE(m_glyph_batch_data.vertex_buffer)
    RESOURCE_RELEASE(m_glyph_batch_data.instance_buffer)
    RESOURCE_RELEASE(m_glyph_batch_data.buffer_array)
    RESOURCE_RELEASE(m_glyph_batch_data.pipeline)

    RESOURCE_RELEASE(m_ninepatch_batch_data.vertex_buffer)
    RESOURCE_RELEASE(m_ninepatch_batch_data.instance_buffer)
    RESOURCE_RELEASE(m_ninepatch_batch_data.buffer_array)
    RESOURCE_RELEASE(m_ninepatch_batch_data.pipeline)

    RESOURCE_RELEASE(m_constant_buffer);
    RESOURCE_RELEASE(m_command_buffer);
    RESOURCE_RELEASE(m_swap_chain);

    LLGL::RenderSystem::Unload(std::move(m_context));
}

void Renderer::InitSpriteBatchPipeline() {
    ZoneScopedN("RenderBatchSprite::init");

    const RenderBackend backend = m_backend;
    const auto& context = m_context;

    const Vertex vertices[] = {
        Vertex(0.0f, 0.0f),
        Vertex(0.0f, 1.0f),
        Vertex(1.0f, 0.0f),
        Vertex(1.0f, 1.0f),
    };

    m_sprite_batch_data.vertex_buffer = CreateVertexBufferInit(sizeof(vertices), vertices, Assets::GetVertexFormat(VertexFormatAsset::SpriteVertex), "SpriteBatch VertexBuffer");

    m_sprite_batch_data.instance_buffer = CreateVertexBuffer(MAX_QUADS * sizeof(SpriteInstance), Assets::GetVertexFormat(VertexFormatAsset::SpriteInstance), "SpriteBatch InstanceBuffer");
    LLGL::Buffer* buffers[] = { m_sprite_batch_data.vertex_buffer, m_sprite_batch_data.instance_buffer };
    m_sprite_batch_data.buffer_array = context->CreateBufferArray(2, buffers);

    LLGL::PipelineLayoutDescriptor pipelineLayoutDesc;
    pipelineLayoutDesc.bindings = {
        LLGL::BindingDescriptor(
            "GlobalUniformBuffer",
            LLGL::ResourceType::Buffer,
            LLGL::BindFlags::ConstantBuffer,
            LLGL::StageFlags::VertexStage,
            LLGL::BindingSlot(2)
        ),
        LLGL::BindingDescriptor("u_texture", LLGL::ResourceType::Texture, LLGL::BindFlags::Sampled, LLGL::StageFlags::FragmentStage, LLGL::BindingSlot(3)),
        LLGL::BindingDescriptor("u_sampler", LLGL::ResourceType::Sampler, 0, LLGL::StageFlags::FragmentStage, LLGL::BindingSlot(backend.IsOpenGL() ? 3 : 4)),
    };

    LLGL::PipelineLayout* pipelineLayout = context->CreatePipelineLayout(pipelineLayoutDesc);

    const ShaderPipeline& sprite_shader = Assets::GetShader(ShaderAsset::SpriteShader);

    LLGL::GraphicsPipelineDescriptor pipelineDesc;
    pipelineDesc.debugName = "SpriteBatch Pipeline";
    pipelineDesc.vertexShader = sprite_shader.vs;
    pipelineDesc.fragmentShader = sprite_shader.ps;
    pipelineDesc.geometryShader = sprite_shader.gs;
    pipelineDesc.pipelineLayout = pipelineLayout;
    pipelineDesc.indexFormat = LLGL::Format::R16UInt;
    pipelineDesc.primitiveTopology = LLGL::PrimitiveTopology::TriangleStrip;
    pipelineDesc.renderPass = m_swap_chain->GetRenderPass();
    pipelineDesc.rasterizer.frontCCW = true;
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

    m_sprite_batch_data.pipeline = context->CreatePipelineState(pipelineDesc);

    pipelineDesc.debugName = "SpriteBatch Pipeline Depth";
    pipelineDesc.depth = LLGL::DepthDescriptor {
        .testEnabled = true,
        .writeEnabled = true,
        .compareOp = LLGL::CompareOp::GreaterEqual,
    };
    m_sprite_batch_data.pipeline_depth = context->CreatePipelineState(pipelineDesc);

    if (const LLGL::Report* report = m_sprite_batch_data.pipeline->GetReport()) {
        if (report->HasErrors()) LOG_ERROR("%s", report->GetText());
    }
}

void Renderer::InitNinepatchBatchPipeline() {
    ZoneScopedN("RenderBatchNinePatch::init");

    const RenderBackend backend = m_backend;
    const auto& context = m_context;

    const Vertex vertices[] = {
        Vertex(0.0f, 0.0f),
        Vertex(0.0f, 1.0f),
        Vertex(1.0f, 0.0f),
        Vertex(1.0f, 1.0f),
    };

    m_ninepatch_batch_data.vertex_buffer = CreateVertexBufferInit(sizeof(vertices), vertices, Assets::GetVertexFormat(VertexFormatAsset::NinePatchVertex), "NinePatchBatch VertexBuffer");
    m_ninepatch_batch_data.instance_buffer = CreateVertexBuffer(MAX_QUADS * sizeof(NinePatchInstance), Assets::GetVertexFormat(VertexFormatAsset::NinePatchInstance), "NinePatchBatch InstanceBuffer");

    {
        LLGL::Buffer* buffers[] = { m_ninepatch_batch_data.vertex_buffer, m_ninepatch_batch_data.instance_buffer };
        m_ninepatch_batch_data.buffer_array = context->CreateBufferArray(2, buffers);
    }

    LLGL::PipelineLayoutDescriptor pipelineLayoutDesc;
    pipelineLayoutDesc.bindings = {
        LLGL::BindingDescriptor(
            "GlobalUniformBuffer",
            LLGL::ResourceType::Buffer,
            LLGL::BindFlags::ConstantBuffer,
            LLGL::StageFlags::VertexStage,
            LLGL::BindingSlot(2)
        ),
        LLGL::BindingDescriptor("u_texture", LLGL::ResourceType::Texture, LLGL::BindFlags::Sampled, LLGL::StageFlags::FragmentStage, LLGL::BindingSlot(3)),
        LLGL::BindingDescriptor("u_sampler", LLGL::ResourceType::Sampler, 0, LLGL::StageFlags::FragmentStage, LLGL::BindingSlot(backend.IsOpenGL() ? 3 : 4)),
    };

    LLGL::PipelineLayout* pipelineLayout = context->CreatePipelineLayout(pipelineLayoutDesc);

    const ShaderPipeline& ninepatch_shader = Assets::GetShader(ShaderAsset::NinePatchShader);

    LLGL::GraphicsPipelineDescriptor pipelineDesc;
    pipelineDesc.debugName = "NinePatchBatch Pipeline";
    pipelineDesc.vertexShader = ninepatch_shader.vs;
    pipelineDesc.fragmentShader = ninepatch_shader.ps;
    pipelineDesc.geometryShader = ninepatch_shader.gs;
    pipelineDesc.pipelineLayout = pipelineLayout;
    pipelineDesc.indexFormat = LLGL::Format::R16UInt;
    pipelineDesc.primitiveTopology = LLGL::PrimitiveTopology::TriangleStrip;
    pipelineDesc.renderPass = m_swap_chain->GetRenderPass();
    pipelineDesc.rasterizer.frontCCW = true;
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

    m_ninepatch_batch_data.pipeline = context->CreatePipelineState(pipelineDesc);

    if (const LLGL::Report* report = m_ninepatch_batch_data.pipeline->GetReport()) {
        if (report->HasErrors()) LOG_ERROR("%s", report->GetText());
    }
}

void Renderer::InitGlyphBatchPipeline() {
    ZoneScopedN("RenderBatchGlyph::init");

    const RenderBackend backend = m_backend;
    const auto& context = m_context;

    const Vertex vertices[] = {
        Vertex(0.0f, 0.0f),
        Vertex(0.0f, 1.0f),
        Vertex(1.0f, 0.0f),
        Vertex(1.0f, 1.0f),
    };

    m_glyph_batch_data.vertex_buffer = CreateVertexBufferInit(sizeof(vertices), vertices, Assets::GetVertexFormat(VertexFormatAsset::FontVertex), "GlyphBatch VertexBuffer");
    m_glyph_batch_data.instance_buffer = CreateVertexBuffer(MAX_GLYPHS * sizeof(GlyphInstance), Assets::GetVertexFormat(VertexFormatAsset::FontInstance), "GlyphBatch InstanceBuffer");

    LLGL::Buffer* buffers[] = { m_glyph_batch_data.vertex_buffer, m_glyph_batch_data.instance_buffer };
    m_glyph_batch_data.buffer_array = context->CreateBufferArray(2, buffers);

    LLGL::PipelineLayoutDescriptor pipelineLayoutDesc;
    pipelineLayoutDesc.bindings = {
        LLGL::BindingDescriptor(
            "GlobalUniformBuffer",
            LLGL::ResourceType::Buffer,
            LLGL::BindFlags::ConstantBuffer,
            LLGL::StageFlags::VertexStage,
            LLGL::BindingSlot(2)
        ),
        LLGL::BindingDescriptor("u_texture", LLGL::ResourceType::Texture, LLGL::BindFlags::Sampled, LLGL::StageFlags::FragmentStage, LLGL::BindingSlot(3)),
        LLGL::BindingDescriptor("u_sampler", LLGL::ResourceType::Sampler, 0, LLGL::StageFlags::FragmentStage, LLGL::BindingSlot(backend.IsOpenGL() ? 3 : 4)),
    };

    LLGL::PipelineLayout* pipelineLayout = context->CreatePipelineLayout(pipelineLayoutDesc);

    const ShaderPipeline& font_shader = Assets::GetShader(ShaderAsset::FontShader);

    LLGL::GraphicsPipelineDescriptor pipelineDesc;
    pipelineDesc.debugName = "GlyphBatch Pipeline";
    pipelineDesc.vertexShader = font_shader.vs;
    pipelineDesc.fragmentShader = font_shader.ps;
    pipelineDesc.geometryShader = font_shader.gs;
    pipelineDesc.pipelineLayout = pipelineLayout;
    pipelineDesc.indexFormat = LLGL::Format::R16UInt;
    pipelineDesc.primitiveTopology = LLGL::PrimitiveTopology::TriangleStrip;
    pipelineDesc.renderPass = m_swap_chain->GetRenderPass();
    pipelineDesc.rasterizer.frontCCW = true;
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

    m_glyph_batch_data.pipeline = context->CreatePipelineState(pipelineDesc);

    if (const LLGL::Report* report = m_glyph_batch_data.pipeline->GetReport()) {
        if (report->HasErrors()) LOG_ERROR("%s", report->GetText());
    }
}
