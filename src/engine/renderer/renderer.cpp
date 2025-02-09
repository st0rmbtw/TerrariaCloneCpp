#include "renderer.hpp"

#include <fstream>
#include <sstream>
#include <tracy/Tracy.hpp>
#include <utility>

#include "../log.hpp"
#include "../utils.hpp"

#include "LLGL/PipelineLayoutFlags.h"
#include "LLGL/PipelineStateFlags.h"
#include "batch.hpp"
#include "macros.hpp"
#include "types.hpp"

using batch_internal::DrawCommand;
using batch_internal::FlushData;
using batch_internal::FlushDataType;

static constexpr size_t MAX_QUADS = 2500;

namespace SpriteFlags {
    enum : uint8_t {
        UI = 0,
        IgnoreCameraZoom,
    };
};

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

    m_sprite_instance_size = 0;
    m_glyph_instance_size = 0;
    m_ninepatch_instance_size = 0;

    m_sprite_instance_count = 0;
    m_glyph_instance_count = 0;
    m_ninepatch_instance_count = 0;
    m_batch_instance_count = 0;

    m_sprite_batch_data.buffer_ptr = m_sprite_batch_data.buffer;
    m_glyph_batch_data.buffer_ptr = m_glyph_batch_data.buffer;
    m_ninepatch_batch_data.buffer_ptr = m_ninepatch_batch_data.buffer;
}

void Renderer::End() {
    ZoneScopedN("Renderer::End");

    m_command_buffer->End();
    m_command_queue->Submit(*m_command_buffer);

    m_swap_chain->Present();
}

void Renderer::ApplyBatchDrawCommands(Batch& batch) {
    ZoneScopedN("Renderer::ApplyBatchDrawCommands");

    Batch::FlushQueue& flush_queue = batch.flush_queue();

    auto* const commands = m_command_buffer;

    int prev_flush_data_type = -1;
    int prev_texture_id = -1;

    LLGL::PipelineState& sprite_pipeline = batch.is_ui()
        ? *m_sprite_batch_data.pipeline_ui
        : batch.depth_enabled()
            ? *m_sprite_batch_data.pipeline_depth
            : *m_sprite_batch_data.pipeline;

    LLGL::PipelineState& glyph_pipeline = batch.is_ui()
        ? *m_glyph_batch_data.pipeline_ui
        : *m_glyph_batch_data.pipeline;

    LLGL::PipelineState& ninepatch_pipeline = batch.is_ui()
        ? *m_ninepatch_batch_data.pipeline_ui
        : *m_ninepatch_batch_data.pipeline;

    size_t offset = 0;

    for (FlushData& flush_data : flush_queue) {
        if (prev_flush_data_type != static_cast<int>(flush_data.type)) {
            switch (flush_data.type) {
            case FlushDataType::Sprite:
                commands->SetVertexBufferArray(*m_sprite_batch_data.buffer_array);
                commands->SetPipelineState(sprite_pipeline);
                offset = batch.sprite_offset();
            break;

            case FlushDataType::Glyph:
                commands->SetVertexBufferArray(*m_glyph_batch_data.buffer_array);
                commands->SetPipelineState(glyph_pipeline);
                offset = batch.glyph_offset();
            break;

            case FlushDataType::NinePatch:
                commands->SetVertexBufferArray(*m_ninepatch_batch_data.buffer_array);
                commands->SetPipelineState(ninepatch_pipeline);
                offset = batch.ninepatch_offset();
            break;
            }

            commands->SetResource(0, *m_constant_buffer);
        }

        if (prev_texture_id != flush_data.texture.id()) {
            commands->SetResource(1, flush_data.texture);
            commands->SetResource(2, Assets::GetSampler(flush_data.texture));
        }
        
        commands->DrawInstanced(4, 0, flush_data.count, offset + flush_data.offset);

        prev_flush_data_type = static_cast<int>(flush_data.type);
        prev_texture_id = flush_data.texture.id();
    }

    flush_queue.clear();
}

void Renderer::SortBatchDrawCommands(Batch& batch) {
    using namespace batch_internal;

    ZoneScopedN("Renderer::SortBatchDrawCommands");

    Batch::DrawCommands& draw_commands = batch.draw_commands();

    std::sort(
        draw_commands.begin(),
        draw_commands.end(),
        [](const DrawCommand& a, const DrawCommand& b) {
            const uint32_t a_order = a.order();
            const uint32_t b_order = b.order();

            if (a_order < b_order) return true;
            if (a_order > b_order) return false;

            return a.texture().id() < b.texture().id();
        }
    );
}

void Renderer::UpdateBatchBuffers(
    Batch& batch,
    size_t begin
) {
    using namespace batch_internal;

    ZoneScopedN("Renderer::UpdateBatchBuffers");

    const Batch::DrawCommands& draw_commands = batch.draw_commands();

    if (draw_commands.empty()) return;

    Batch::FlushQueue& flush_queue = batch.flush_queue();

    Texture sprite_prev_texture;
    uint32_t sprite_count = 0;
    uint32_t sprite_total_count = 0;
    uint32_t sprite_vertex_offset = 0;
    uint32_t sprite_remaining = batch.sprite_count();

    Texture glyph_prev_texture;
    uint32_t glyph_count = 0;
    uint32_t glyph_total_count = 0;
    uint32_t glyph_vertex_offset = 0;
    uint32_t glyph_remaining = batch.glyph_count();

    Texture ninepatch_prev_texture;
    uint32_t ninepatch_count = 0;
    uint32_t ninepatch_total_count = 0;
    uint32_t ninepatch_vertex_offset = 0;
    uint32_t ninepatch_remaining = batch.ninepatch_count();

    uint32_t prev_order = draw_commands[begin].order();

    size_t i = 0;
    for (i = begin; i < draw_commands.size(); ++i) {
        if (m_batch_instance_count >= MAX_QUADS) {
            break;
        }

        const batch_internal::DrawCommand& draw_command = draw_commands[i];

        switch (draw_command.type()) {
        case DrawCommand::DrawSprite: {
            const DrawCommandSprite& sprite_data = draw_command.sprite_data();
            
            if (sprite_remaining == 0) continue;

            if (sprite_total_count == 0) {
                sprite_prev_texture = sprite_data.texture;
            }

            const uint32_t prev_texture_id = sprite_prev_texture.id();
            const uint32_t curr_texture_id = sprite_data.texture.id();

            const uint32_t new_order = sprite_data.order;

            const bool needs_flush = prev_texture_id != curr_texture_id || prev_order != new_order;

            if (sprite_count > 0 && needs_flush) {
                flush_queue.push_back(FlushData {
                    .texture = sprite_prev_texture,
                    .offset = sprite_vertex_offset,
                    .count = sprite_count,
                    .order = prev_order,
                    .type = FlushDataType::Sprite
                });
                sprite_count = 0;
                sprite_vertex_offset = sprite_total_count;
            }

            int flags = sprite_data.ignore_camera_zoom << SpriteFlags::IgnoreCameraZoom;

            m_sprite_batch_data.buffer_ptr->position = sprite_data.position;
            m_sprite_batch_data.buffer_ptr->rotation = sprite_data.rotation;
            m_sprite_batch_data.buffer_ptr->size = sprite_data.size;
            m_sprite_batch_data.buffer_ptr->offset = sprite_data.offset;
            m_sprite_batch_data.buffer_ptr->uv_offset_scale = sprite_data.uv_offset_scale;
            m_sprite_batch_data.buffer_ptr->color = sprite_data.color;
            m_sprite_batch_data.buffer_ptr->outline_color = sprite_data.outline_color;
            m_sprite_batch_data.buffer_ptr->outline_thickness = sprite_data.outline_thickness;
            m_sprite_batch_data.buffer_ptr->flags = flags;
            m_sprite_batch_data.buffer_ptr++;

            ++sprite_count;
            ++sprite_total_count;
            --sprite_remaining;

            if (sprite_remaining == 0) {
                flush_queue.push_back(FlushData {
                    .texture = sprite_data.texture,
                    .offset = sprite_vertex_offset,
                    .count = sprite_count,
                    .order = sprite_data.order,
                    .type = FlushDataType::Sprite
                });
                sprite_count = 0;
            }

            sprite_prev_texture = sprite_data.texture;
            prev_order = new_order;
        } break;
        case DrawCommand::DrawGlyph: {
            const DrawCommandGlyph& glyph_data = draw_command.glyph_data();

            if (glyph_remaining == 0) continue;

            if (glyph_total_count == 0) {
                glyph_prev_texture = glyph_data.texture;
            }

            const uint32_t new_order = glyph_data.order;

            const bool needs_flush = glyph_prev_texture.id() != glyph_data.texture.id() || prev_order != new_order;

            if (glyph_count > 0 && needs_flush) {
                flush_queue.push_back(FlushData {
                    .texture = glyph_prev_texture,
                    .offset = glyph_vertex_offset,
                    .count = glyph_count,
                    .order = prev_order,
                    .type = FlushDataType::Glyph
                });
                glyph_count = 0;
                glyph_vertex_offset = glyph_total_count;
            }

            m_glyph_batch_data.buffer_ptr->color = glyph_data.color;
            m_glyph_batch_data.buffer_ptr->pos = glyph_data.pos;
            m_glyph_batch_data.buffer_ptr->size = glyph_data.size;
            m_glyph_batch_data.buffer_ptr->tex_size = glyph_data.tex_size;
            m_glyph_batch_data.buffer_ptr->uv = glyph_data.tex_uv;
            m_glyph_batch_data.buffer_ptr++;

            ++glyph_count;
            ++glyph_total_count;
            --glyph_remaining;

            if (glyph_remaining == 0) {
                flush_queue.push_back(FlushData {
                    .texture = glyph_data.texture,
                    .offset = glyph_vertex_offset,
                    .count = glyph_count,
                    .order = glyph_data.order,
                    .type = FlushDataType::Glyph
                });
                glyph_count = 0;
            }

            glyph_prev_texture = glyph_data.texture;
            prev_order = new_order;
        } break;
        case DrawCommand::DrawNinePatch: {
            const DrawCommandNinePatch& ninepatch_data = draw_command.ninepatch_data();

            if (ninepatch_remaining == 0) continue;

            if (ninepatch_total_count == 0) {
                ninepatch_prev_texture = ninepatch_data.texture;
            }

            const uint32_t prev_texture_id = ninepatch_prev_texture.id();
            const uint32_t curr_texture_id = ninepatch_data.texture.id();

            const uint32_t new_order = ninepatch_data.order;

            const bool needs_flush = prev_texture_id != curr_texture_id || prev_order != new_order;

            if (ninepatch_count > 0 && needs_flush) {
                flush_queue.push_back(FlushData {
                    .texture = ninepatch_prev_texture,
                    .offset = ninepatch_vertex_offset,
                    .count = ninepatch_count,
                    .order = prev_order,
                    .type = FlushDataType::NinePatch
                });
                ninepatch_count = 0;
                ninepatch_vertex_offset = ninepatch_total_count;
            }

            m_ninepatch_batch_data.buffer_ptr->position = ninepatch_data.position;
            m_ninepatch_batch_data.buffer_ptr->rotation = ninepatch_data.rotation;
            m_ninepatch_batch_data.buffer_ptr->margin = ninepatch_data.margin;
            m_ninepatch_batch_data.buffer_ptr->size = ninepatch_data.size;
            m_ninepatch_batch_data.buffer_ptr->offset = ninepatch_data.offset;
            m_ninepatch_batch_data.buffer_ptr->source_size = ninepatch_data.source_size;
            m_ninepatch_batch_data.buffer_ptr->output_size = ninepatch_data.output_size;
            m_ninepatch_batch_data.buffer_ptr->uv_offset_scale = ninepatch_data.uv_offset_scale;
            m_ninepatch_batch_data.buffer_ptr->color = ninepatch_data.color;
            m_ninepatch_batch_data.buffer_ptr->flags = 0;
            m_ninepatch_batch_data.buffer_ptr++;

            ++ninepatch_count;
            ++ninepatch_total_count;
            --ninepatch_remaining;

            if (ninepatch_remaining == 0) {
                flush_queue.push_back(FlushData {
                    .texture = ninepatch_data.texture,
                    .offset = ninepatch_vertex_offset,
                    .count = ninepatch_count,
                    .order = ninepatch_data.order,
                    .type = FlushDataType::NinePatch
                });
                ninepatch_count = 0;
            }

            ninepatch_prev_texture = ninepatch_data.texture;
            prev_order = new_order;
        } break;
        }

        if (m_batch_instance_count + 1 >= MAX_QUADS) {
            if (sprite_count > 0) {
                flush_queue.push_back(FlushData {
                    .texture = sprite_prev_texture,
                    .offset = sprite_vertex_offset,
                    .count = sprite_count,
                    .order = prev_order,
                    .type = FlushDataType::Sprite
                });
            }

            if (glyph_count > 0) {
                flush_queue.push_back(FlushData {
                    .texture = glyph_prev_texture,
                    .offset = glyph_vertex_offset,
                    .count = glyph_count,
                    .order = prev_order,
                    .type = FlushDataType::Glyph
                });
            }

            if (ninepatch_count > 0) {
                flush_queue.push_back(FlushData {
                    .texture = ninepatch_prev_texture,
                    .offset = ninepatch_vertex_offset,
                    .count = ninepatch_count,
                    .order = prev_order,
                    .type = FlushDataType::NinePatch
                });
            }
        }

        ++m_batch_instance_count;
    }

    batch.set_draw_commands_done(i);

    const size_t sprite_size = sprite_total_count * sizeof(SpriteInstance);
    m_sprite_instance_size += sprite_size;
    m_sprite_instance_count += sprite_total_count;
    batch.set_sprite_count(sprite_remaining);

    const size_t glyph_size = glyph_total_count * sizeof(GlyphInstance);
    m_glyph_instance_size += glyph_size;
    m_glyph_instance_count += glyph_total_count;
    batch.set_glyph_count(glyph_remaining);

    const size_t ninepatch_size = ninepatch_total_count * sizeof(NinePatchInstance);
    m_ninepatch_instance_size += ninepatch_size;
    m_ninepatch_instance_count += ninepatch_total_count;
    batch.set_ninepatch_count(ninepatch_remaining);
}


void Renderer::PrepareBatch(Batch& batch) {
    if (batch.draw_commands().empty()) return;

    batch.set_sprite_offset(m_sprite_instance_count);
    batch.set_glyph_offset(m_glyph_instance_count);
    batch.set_ninepatch_offset(m_ninepatch_instance_count);

    SortBatchDrawCommands(batch);
    UpdateBatchBuffers(batch);
}

void Renderer::UploadBatchData() {
    if (m_sprite_instance_size > 0) {
        UpdateBuffer(m_sprite_batch_data.instance_buffer, m_sprite_batch_data.buffer, m_sprite_instance_size);
    }

    if (m_glyph_instance_size > 0) {
        UpdateBuffer(m_glyph_batch_data.instance_buffer, m_glyph_batch_data.buffer, m_glyph_instance_size);
    }

    if (m_ninepatch_instance_size > 0) {
        UpdateBuffer(m_ninepatch_batch_data.instance_buffer, m_ninepatch_batch_data.buffer, m_ninepatch_instance_size);
    }
}

void Renderer::RenderBatch(Batch& batch) {
    ZoneScopedN("Renderer::RenderBatch");

    const Batch::DrawCommands& draw_commands = batch.draw_commands();

    if (draw_commands.empty()) return;

    ApplyBatchDrawCommands(batch);

    while (batch.draw_commands_done() < draw_commands.size()) {
        m_batch_instance_count = 0;

        m_sprite_instance_count = 0;
        m_sprite_instance_size = 0;
        m_sprite_batch_data.buffer_ptr = m_sprite_batch_data.buffer;
        batch.set_sprite_offset(m_sprite_instance_count);

        m_glyph_instance_count = 0;
        m_glyph_instance_size = 0;
        m_glyph_batch_data.buffer_ptr = m_glyph_batch_data.buffer;
        batch.set_glyph_offset(m_glyph_instance_count);

        m_ninepatch_instance_count = 0;
        m_ninepatch_instance_size = 0;
        m_ninepatch_batch_data.buffer_ptr = m_ninepatch_batch_data.buffer;
        batch.set_ninepatch_offset(m_ninepatch_instance_count);

        UpdateBatchBuffers(batch, batch.draw_commands_done());
        UploadBatchData();

        ApplyBatchDrawCommands(batch);
    }
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

    m_sprite_batch_data.buffer = checked_alloc<SpriteInstance>(MAX_QUADS);
    m_sprite_batch_data.buffer_ptr = m_sprite_batch_data.buffer;

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
    const ShaderPipeline& sprite_shader_ui = Assets::GetShader(ShaderAsset::UiSpriteShader);

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

    {
        LLGL::GraphicsPipelineDescriptor depthPipelineDesc = pipelineDesc;
        depthPipelineDesc.debugName = "SpriteBatch Pipeline Depth";
        depthPipelineDesc.depth = LLGL::DepthDescriptor {
            .testEnabled = true,
            .writeEnabled = true,
            .compareOp = LLGL::CompareOp::GreaterEqual,
        };
        m_sprite_batch_data.pipeline_depth = context->CreatePipelineState(depthPipelineDesc);
    }

    {
        LLGL::GraphicsPipelineDescriptor uiPipelineDesc = pipelineDesc;
        uiPipelineDesc.debugName = "SpriteBatch Pipeline UI";
        uiPipelineDesc.vertexShader = sprite_shader_ui.vs;
        uiPipelineDesc.fragmentShader = sprite_shader_ui.ps;
        uiPipelineDesc.geometryShader = sprite_shader_ui.gs;
        m_sprite_batch_data.pipeline_ui = context->CreatePipelineState(uiPipelineDesc);
    }

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

    m_ninepatch_batch_data.buffer = checked_alloc<NinePatchInstance>(MAX_QUADS);
    m_ninepatch_batch_data.buffer_ptr = m_ninepatch_batch_data.buffer;

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
    const ShaderPipeline& ninepatch_shader_ui = Assets::GetShader(ShaderAsset::UiNinePatchShader);

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

    pipelineDesc.debugName = "NinePatchBatch Pipeline UI";
    pipelineDesc.vertexShader = ninepatch_shader_ui.vs;
    pipelineDesc.fragmentShader = ninepatch_shader_ui.ps;

    m_ninepatch_batch_data.pipeline_ui = context->CreatePipelineState(pipelineDesc);

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

    m_glyph_batch_data.buffer = checked_alloc<GlyphInstance>(MAX_QUADS);
    m_glyph_batch_data.buffer_ptr = m_glyph_batch_data.buffer;

    m_glyph_batch_data.vertex_buffer = CreateVertexBufferInit(sizeof(vertices), vertices, Assets::GetVertexFormat(VertexFormatAsset::FontVertex), "GlyphBatch VertexBuffer");
    m_glyph_batch_data.instance_buffer = CreateVertexBuffer(MAX_QUADS * sizeof(GlyphInstance), Assets::GetVertexFormat(VertexFormatAsset::FontInstance), "GlyphBatch InstanceBuffer");

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
    const ShaderPipeline& font_shader_ui = Assets::GetShader(ShaderAsset::UiFontShader);

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

    pipelineDesc.debugName = "GlyphBatch Pipeline UI";
    pipelineDesc.vertexShader = font_shader_ui.vs;
    pipelineDesc.fragmentShader = font_shader_ui.ps;
    pipelineDesc.geometryShader = font_shader_ui.gs;

    m_glyph_batch_data.pipeline_ui = context->CreatePipelineState(pipelineDesc);

    if (const LLGL::Report* report = m_glyph_batch_data.pipeline->GetReport()) {
        if (report->HasErrors()) LOG_ERROR("%s", report->GetText());
    }
}
