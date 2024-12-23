#include "renderer.hpp"

#include <memory>
#include <sstream>
#include <fstream>

#include <LLGL/Utils/TypeNames.h>
#include <LLGL/CommandBufferFlags.h>
#include <LLGL/PipelineStateFlags.h>
#include <LLGL/RendererConfiguration.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include "../assets.hpp"
#include "../log.hpp"
#include "../types/shader_type.hpp"
#include "../utils.hpp"

#include "types.hpp"
#include "utils.hpp"
#include "batch.hpp"
#include "world_renderer.hpp"
#include "background_renderer.hpp"
#include "particle_renderer.hpp"

static struct RendererState {
    LLGL::RenderSystemPtr context = nullptr;
    LLGL::SwapChain* swap_chain = nullptr;
    LLGL::CommandBuffer* command_buffer = nullptr;
    LLGL::CommandQueue* command_queue = nullptr;
    std::shared_ptr<CustomSurface> surface = nullptr;
#if DEBUG
    LLGL::RenderingDebugger* debugger = nullptr;
#endif

    LLGL::Buffer* constant_buffer = nullptr;
    LLGL::RenderPass* render_pass = nullptr;
    LLGL::RenderPass* clear_render_pass = nullptr;

    LLGL::Buffer* chunk_vertex_buffer = nullptr;
    LLGL::RenderTarget* world_render_target = nullptr;
    LLGL::Texture* world_render_texture = nullptr;
    LLGL::Texture* world_depth_texture = nullptr;

    LLGL::Texture* background_render_texture = nullptr;
    LLGL::RenderTarget* background_render_target = nullptr;

    LLGL::PipelineState* postprocess_pipeline = nullptr;
    LLGL::Buffer* fullscreen_triangle_vertex_buffer = nullptr;
    
    RenderBatchSprite sprite_batch;
    RenderBatchGlyph glyph_batch;
    WorldRenderer world_renderer;
    BackgroundRenderer background_renderer;
    ParticleRenderer particle_renderer;
    RenderBackend backend;
    
    uint32_t main_depth_index = 0;
    uint32_t world_depth_index = 0;
    uint32_t ui_depth_index = 0;

    uint32_t texture_index = 0;

    math::Rect ui_frustum;
    math::Rect nonscale_camera_frustum;
    math::Rect camera_frustum;
} state;

const LLGL::RenderSystemPtr& Renderer::Context() { return state.context; }
LLGL::SwapChain* Renderer::SwapChain() { return state.swap_chain; }
LLGL::CommandBuffer* Renderer::CommandBuffer() { return state.command_buffer; }
LLGL::CommandQueue* Renderer::CommandQueue() { return state.command_queue; }
const std::shared_ptr<CustomSurface>& Renderer::Surface() { return state.surface; }
LLGL::Buffer* Renderer::GlobalUniformBuffer() { return state.constant_buffer; }
RenderBackend Renderer::Backend() { return state.backend; }
uint32_t Renderer::GetMainDepthIndex() { return state.main_depth_index; };
uint32_t Renderer::GetWorldDepthIndex() { return state.world_depth_index; };
uint32_t Renderer::GetUiDepthIndex() { return state.ui_depth_index; };
const LLGL::RenderPass* Renderer::DefaultRenderPass() { return state.render_pass; };
LLGL::Buffer* Renderer::ChunkVertexBuffer() { return state.chunk_vertex_buffer; }

bool Renderer::InitEngine(RenderBackend backend) {
    LLGL::Report report;

    void* configPtr = nullptr;
    size_t configSize = 0;

    if (backend.IsOpenGL()) {
        LLGL::RendererConfigurationOpenGL config;
        config.majorVersion = 4;
        config.minorVersion = 3;
        config.contextProfile = LLGL::OpenGLContextProfile::CoreProfile;
        configPtr = &config;
        configSize = sizeof(config);
    }

    LLGL::RenderSystemDescriptor rendererDesc;
    rendererDesc.moduleName = backend.ToString();
    rendererDesc.rendererConfig = configPtr;
    rendererDesc.rendererConfigSize = configSize;

#if DEBUG
    state.debugger = new LLGL::RenderingDebugger();
    rendererDesc.flags    = LLGL::RenderSystemFlags::DebugDevice;
    rendererDesc.debugger = state.debugger;
#endif

    state.context = LLGL::RenderSystem::Load(rendererDesc, &report);
    state.backend = backend;

    if (report.HasErrors()) {
        LOG_ERROR("%s", report.GetText());
        return false;
    }

    const auto& info = state.context->GetRendererInfo();

    LOG_INFO("Renderer:             %s", info.rendererName.c_str());
    LOG_INFO("Device:               %s", info.deviceName.c_str());
    LOG_INFO("Vendor:               %s", info.vendorName.c_str());
    LOG_INFO("Shading Language:     %s", info.shadingLanguageName.c_str());

    LOG_INFO("Extensions:");
    for (const std::string& extension : info.extensionNames) {
        LOG_INFO("  %s", extension.c_str());
    }

    return true;
}

bool Renderer::Init(GLFWwindow* window, const LLGL::Extent2D& resolution, bool vsync, bool fullscreen) {
    const LLGL::RenderSystemPtr& context = state.context;
    const RenderBackend backend = state.backend;

    state.surface = std::make_shared<CustomSurface>(window, resolution);

    LLGL::SwapChainDescriptor swapChainDesc;
    swapChainDesc.resolution = resolution;
    swapChainDesc.fullscreen = fullscreen;

    state.swap_chain = context->CreateSwapChain(swapChainDesc, state.surface);
    state.swap_chain->SetVsyncInterval(vsync);

    state.command_buffer = context->CreateCommandBuffer();
    state.command_queue = context->GetCommandQueue();

    state.constant_buffer = CreateConstantBuffer(sizeof(ProjectionsUniform));

    {
        LLGL::RenderPassDescriptor render_pass_desc;
        render_pass_desc.colorAttachments[0].format = state.swap_chain->GetColorFormat();
        render_pass_desc.colorAttachments[0].loadOp = LLGL::AttachmentLoadOp::Load;
        render_pass_desc.colorAttachments[0].storeOp = LLGL::AttachmentStoreOp::Store;

        render_pass_desc.depthAttachment.format = state.swap_chain->GetDepthStencilFormat();
        render_pass_desc.depthAttachment.loadOp = LLGL::AttachmentLoadOp::Load;
        render_pass_desc.depthAttachment.storeOp = LLGL::AttachmentStoreOp::Store;

        render_pass_desc.stencilAttachment.format = state.swap_chain->GetDepthStencilFormat();
        render_pass_desc.stencilAttachment.loadOp = LLGL::AttachmentLoadOp::Undefined;
        render_pass_desc.stencilAttachment.storeOp = LLGL::AttachmentStoreOp::Undefined;
        state.render_pass = context->CreateRenderPass(render_pass_desc);
    }
    {
        LLGL::RenderPassDescriptor render_pass_desc;
        render_pass_desc.colorAttachments[0].format = state.swap_chain->GetColorFormat();
        render_pass_desc.colorAttachments[0].loadOp = LLGL::AttachmentLoadOp::Clear;
        render_pass_desc.colorAttachments[0].storeOp = LLGL::AttachmentStoreOp::Store;

        render_pass_desc.depthAttachment.format = state.swap_chain->GetDepthStencilFormat();
        render_pass_desc.depthAttachment.loadOp = LLGL::AttachmentLoadOp::Clear;
        render_pass_desc.depthAttachment.storeOp = LLGL::AttachmentStoreOp::Store;

        render_pass_desc.stencilAttachment.format = state.swap_chain->GetDepthStencilFormat();
        render_pass_desc.stencilAttachment.loadOp = LLGL::AttachmentLoadOp::Undefined;
        render_pass_desc.stencilAttachment.storeOp = LLGL::AttachmentStoreOp::Undefined;
        state.clear_render_pass = context->CreateRenderPass(render_pass_desc);
    }

    state.sprite_batch.init();
    state.glyph_batch.init();
    state.world_renderer.init();
    state.background_renderer.init();
    state.particle_renderer.init();

    const glm::vec2 tile_tex_size = glm::vec2(Assets::GetTexture(TextureAsset::Tiles).size);
    const glm::vec2 wall_tex_size = glm::vec2(Assets::GetTexture(TextureAsset::Walls).size);

    const glm::vec2 tile_padding = glm::vec2(Constants::TILE_TEXTURE_PADDING) / tile_tex_size;
    const glm::vec2 wall_padding = glm::vec2(Constants::WALL_TEXTURE_PADDING) / wall_tex_size;

    const ChunkVertex vertices[] = {
        ChunkVertex(0.0, 0.0, wall_tex_size, tile_tex_size, wall_padding, tile_padding),
        ChunkVertex(0.0, 1.0, wall_tex_size, tile_tex_size, wall_padding, tile_padding),
        ChunkVertex(1.0, 0.0, wall_tex_size, tile_tex_size, wall_padding, tile_padding),
        ChunkVertex(1.0, 1.0, wall_tex_size, tile_tex_size, wall_padding, tile_padding),
    };

    state.chunk_vertex_buffer = CreateVertexBufferInit(sizeof(vertices), vertices, Assets::GetVertexFormat(VertexFormatAsset::TilemapVertex), "WorldRenderer VertexBuffer");

    LLGL::PipelineLayoutDescriptor pipelineLayoutDesc;
    pipelineLayoutDesc.bindings = {
        LLGL::BindingDescriptor(
            "GlobalUniformBuffer",
            LLGL::ResourceType::Buffer,
            LLGL::BindFlags::ConstantBuffer,
            LLGL::StageFlags::VertexStage,
            LLGL::BindingSlot(2)
        ),

        LLGL::BindingDescriptor("u_background_texture", LLGL::ResourceType::Texture, LLGL::BindFlags::Sampled, LLGL::StageFlags::FragmentStage, LLGL::BindingSlot(3)),
        LLGL::BindingDescriptor("u_background_sampler", LLGL::ResourceType::Sampler, 0, LLGL::StageFlags::FragmentStage, backend.IsOpenGL() ? 3 : 4),

        LLGL::BindingDescriptor("u_world_texture", LLGL::ResourceType::Texture, LLGL::BindFlags::Sampled, LLGL::StageFlags::FragmentStage, LLGL::BindingSlot(5)),
        LLGL::BindingDescriptor("u_world_sampler", LLGL::ResourceType::Sampler, 0, LLGL::StageFlags::FragmentStage, backend.IsOpenGL() ? 5 : 6),

        LLGL::BindingDescriptor("u_light_texture", LLGL::ResourceType::Texture, LLGL::BindFlags::Sampled, LLGL::StageFlags::FragmentStage, LLGL::BindingSlot(7)),
        LLGL::BindingDescriptor("u_light_sampler", LLGL::ResourceType::Sampler, 0, LLGL::StageFlags::FragmentStage, backend.IsOpenGL() ? 7 : 8),
    };

    LLGL::PipelineLayout* pipelineLayout = context->CreatePipelineLayout(pipelineLayoutDesc);

    const ShaderPipeline& postprocess_shader = Assets::GetShader(ShaderAsset::PostProcessShader);

    LLGL::GraphicsPipelineDescriptor pipelineDesc;
    pipelineDesc.debugName = "LightMap Pipeline";
    pipelineDesc.vertexShader = postprocess_shader.vs;
    pipelineDesc.fragmentShader = postprocess_shader.ps;
    pipelineDesc.pipelineLayout = pipelineLayout;
    pipelineDesc.indexFormat = LLGL::Format::Undefined;
    pipelineDesc.primitiveTopology = LLGL::PrimitiveTopology::TriangleStrip;
    pipelineDesc.rasterizer.frontCCW = true;

    state.postprocess_pipeline = context->CreatePipelineState(pipelineDesc);

    ResizeTextures(resolution);

    return true;
}

void Renderer::ResizeTextures(LLGL::Extent2D resolution) {
    const LLGL::RenderSystemPtr& context = state.context;
    const auto* swap_chain = state.swap_chain;

    if (state.world_render_texture) context->Release(*state.world_render_texture);
    if (state.world_depth_texture) context->Release(*state.world_depth_texture);
    if (state.background_render_texture) context->Release(*state.background_render_texture);

    if (state.world_render_target) context->Release(*state.world_render_target);
    if (state.background_render_target) context->Release(*state.background_render_target);

    state.world_render_texture = nullptr;
    state.background_render_texture = nullptr;
    state.world_render_target = nullptr;
    state.background_render_target = nullptr;

    LLGL::TextureDescriptor texture_desc;
    texture_desc.extent = LLGL::Extent3D(resolution.width, resolution.height, 1);
    texture_desc.format = swap_chain->GetColorFormat();
    texture_desc.miscFlags = 0;
    texture_desc.cpuAccessFlags = 0;

    LLGL::TextureDescriptor depth_texture_desc;
    depth_texture_desc.extent = LLGL::Extent3D(resolution.width, resolution.height, 1);
    depth_texture_desc.format = swap_chain->GetDepthStencilFormat();
    depth_texture_desc.bindFlags = LLGL::BindFlags::DepthStencilAttachment;
    depth_texture_desc.miscFlags = 0;
    depth_texture_desc.cpuAccessFlags = 0;

    state.world_depth_texture = context->CreateTexture(depth_texture_desc);
    state.world_render_texture = context->CreateTexture(texture_desc);
    state.background_render_texture = context->CreateTexture(texture_desc);
    
    LLGL::RenderTargetDescriptor render_target_desc;
    render_target_desc.resolution = resolution;
    render_target_desc.colorAttachments[0] = state.world_render_texture;
    render_target_desc.depthStencilAttachment = state.world_depth_texture;
    state.world_render_target = context->CreateRenderTarget(render_target_desc);
    
    LLGL::RenderTargetDescriptor background_target_desc;
    background_target_desc.resolution = resolution;
    background_target_desc.colorAttachments[0] = state.background_render_texture;
    background_target_desc.depthStencilAttachment.format = swap_chain->GetDepthStencilFormat();
    state.background_render_target = context->CreateRenderTarget(background_target_desc);
}

void Renderer::InitWorldRenderer(const WorldData &world) {
    using Constants::SUBDIVISION;
    using Constants::TILE_SIZE;

    const auto& context = state.context;

    if (state.fullscreen_triangle_vertex_buffer) context->Release(*state.fullscreen_triangle_vertex_buffer);

    const glm::vec2 world_size = glm::vec2(world.area.size()) * TILE_SIZE;

    const glm::vec2 vertices[] = {
        glm::vec2(-1.0f, 1.0f),  glm::vec2(0.0f, 0.0f), world_size,
        glm::vec2(3.0f,  1.0f),  glm::vec2(2.0f, 0.0f), world_size,
        glm::vec2(-1.0f, -3.0f), glm::vec2(0.0f, 2.0f), world_size,
    };
    state.fullscreen_triangle_vertex_buffer = CreateVertexBufferInit(sizeof(vertices), vertices, Assets::GetVertexFormat(VertexFormatAsset::PostProcessVertex));

    state.world_renderer.init_lightmap_texture(world);
}

void Renderer::Begin(const Camera& camera, WorldData& world) {
    auto* const commands = Renderer::CommandBuffer();

    const math::Rect camera_frustum = math::Rect::from_corners(
        camera.position() + camera.get_projection_area().min,
        camera.position() + camera.get_projection_area().max
    );
    const math::Rect nonscale_camera_frustum = math::Rect::from_corners(
        camera.position() + camera.get_nonscale_projection_area().min,
        camera.position() + camera.get_nonscale_projection_area().max
    );
    const math::Rect ui_frustum = math::Rect::from_corners(glm::vec2(0.0), camera.viewport());

    state.camera_frustum = camera_frustum;
    state.nonscale_camera_frustum = nonscale_camera_frustum;
    state.ui_frustum = ui_frustum;

    for (auto it = world.lightmap_tasks.cbegin(); it != world.lightmap_tasks.cend();) {
        const LightMapTask& task = *it;
        LightMapTaskResult result = task.result->load();

        if (result.is_complete) {
            state.world_renderer.update_lightmap_texture(world, result);
            delete[] result.data;
            delete[] result.mask;
            it = world.lightmap_tasks.erase(it);
            continue;
        }

        ++it;
    }

    commands->Begin();
    commands->SetViewport(state.swap_chain->GetResolution());

    state.sprite_batch.begin();
    state.glyph_batch.begin();

    state.main_depth_index = 0;
    state.world_depth_index = 0;
    state.ui_depth_index = 0;
}

void Renderer::Render(const Camera& camera, const ChunkManager& chunk_manager) {
    auto* const commands = Renderer::CommandBuffer();
    auto* const queue = state.command_queue;

    auto projections_uniform = ProjectionsUniform {
        .screen_projection_matrix = camera.get_screen_projection_matrix(),
        .view_projection_matrix = camera.get_view_projection_matrix(),
        .nonscale_view_projection_matrix = camera.get_nonscale_view_projection_matrix(),
        .nonscale_projection_matrix = camera.get_nonscale_projection_matrix(),
        .transform_matrix = camera.get_transform_matrix(),
        .inv_view_proj_matrix = camera.get_inv_view_projection_matrix(),
        .camera_position = camera.position(),
        .window_size = camera.viewport(),
        .max_depth = static_cast<float>(state.main_depth_index + state.ui_depth_index),
        // The first three are reserved for background, walls and tiles
        .max_world_depth = static_cast<float>(3 + state.world_depth_index),
    };

    commands->UpdateBuffer(*state.constant_buffer, 0, &projections_uniform, sizeof(projections_uniform));

    state.particle_renderer.compute();

    LLGL::ClearValue clear_value[] = {
        LLGL::ClearValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
        LLGL::ClearValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f)
    };

    commands->BeginRenderPass(*state.background_render_target, state.clear_render_pass, 2, clear_value);
        state.background_renderer.render();
    commands->EndRenderPass();

    commands->BeginRenderPass(*state.world_render_target, state.clear_render_pass, 2, clear_value);
        state.background_renderer.render_world();
        state.world_renderer.render(chunk_manager);
        state.sprite_batch.render_world();
    commands->EndRenderPass();

    commands->BeginRenderPass(*state.swap_chain, Renderer::DefaultRenderPass());
        commands->Clear(LLGL::ClearFlags::ColorDepth, LLGL::ClearValue(1.0f, 0.0f, 0.0f, 1.0f, 0.0f));

        commands->SetVertexBuffer(*state.fullscreen_triangle_vertex_buffer);
        commands->SetPipelineState(*state.postprocess_pipeline);
        commands->SetResource(0, *state.constant_buffer);

        commands->SetResource(1, *state.background_render_texture);
        commands->SetResource(2, Assets::GetSampler(TextureSampler::Nearest));

        commands->SetResource(3, *state.world_render_texture);
        commands->SetResource(4, Assets::GetSampler(TextureSampler::Nearest));

        commands->SetResource(5, *state.world_renderer.lightmap_texture());
        commands->SetResource(6, Assets::GetSampler(TextureSampler::Nearest));
        commands->Draw(3, 0);

        state.particle_renderer.render();
        state.sprite_batch.render();
        state.glyph_batch.render();
    commands->EndRenderPass();

    commands->End();
    queue->Submit(*commands);

    state.swap_chain->Present();
}

#if DEBUG
void Renderer::PrintDebugInfo() {
    LLGL::FrameProfile profile;
    state.debugger->FlushProfile(&profile);

    LOG_DEBUG("Draw commands count: %u", profile.commandBufferRecord.drawCommands);
}
#endif

inline void add_sprite_to_batch(const Sprite& sprite, RenderLayer layer, bool is_ui, int depth) {
    glm::vec4 uv_offset_scale = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);

    if (sprite.flip_x()) {
        uv_offset_scale.x += uv_offset_scale.z;
        uv_offset_scale.z *= -1.0f;
    }

    if (sprite.flip_y()) {
        uv_offset_scale.y += uv_offset_scale.w;
        uv_offset_scale.w *= -1.0f;
    }

    state.sprite_batch.draw_sprite(sprite, uv_offset_scale, sprite.texture(), layer, is_ui, depth);
}

inline void add_atlas_sprite_to_batch(const TextureAtlasSprite& sprite, RenderLayer layer, bool is_ui, int depth) {
    const math::Rect& rect = sprite.atlas().get_rect(sprite.index());

    glm::vec4 uv_offset_scale = glm::vec4(
        rect.min.x / sprite.atlas().texture().size.x,
        rect.min.y / sprite.atlas().texture().size.y,
        rect.size().x / sprite.atlas().texture().size.x,
        rect.size().y / sprite.atlas().texture().size.y
    );

    if (sprite.flip_x()) {
        uv_offset_scale.x += uv_offset_scale.z;
        uv_offset_scale.z *= -1.0f;
    }

    if (sprite.flip_y()) {
        uv_offset_scale.y += uv_offset_scale.w;
        uv_offset_scale.w *= -1.0f;
    }

    state.sprite_batch.draw_sprite(sprite, uv_offset_scale, sprite.atlas().texture(), layer, is_ui, depth);
}

void Renderer::DrawSprite(const Sprite& sprite, RenderLayer render_layer, int depth) {
    const math::Rect aabb = sprite.calculate_aabb();

    if (sprite.nonscalable() && !state.nonscale_camera_frustum.intersects(aabb)) return;
    if (!sprite.nonscalable() && !state.camera_frustum.intersects(aabb)) return;

    add_sprite_to_batch(sprite, render_layer, false, depth);
}

void Renderer::DrawSpriteUI(const Sprite& sprite, int depth) {
    const math::Rect aabb = sprite.calculate_aabb();
    if (!state.ui_frustum.intersects(aabb)) return;

    add_sprite_to_batch(sprite, RenderLayer::Main, true, depth);
}

void Renderer::DrawAtlasSprite(const TextureAtlasSprite& sprite, RenderLayer render_layer, int depth) {
    const math::Rect aabb = sprite.calculate_aabb();

    if (sprite.nonscalable() && !state.nonscale_camera_frustum.intersects(aabb)) return;
    if (!sprite.nonscalable() && !state.camera_frustum.intersects(aabb)) return;

    add_atlas_sprite_to_batch(sprite, render_layer, false, depth);
}

void Renderer::DrawAtlasSpriteUI(const TextureAtlasSprite& sprite, int depth) {
    const math::Rect aabb = sprite.calculate_aabb();
    if (!state.ui_frustum.intersects(aabb)) return;

    add_atlas_sprite_to_batch(sprite, RenderLayer::Main, true, depth);
}

void Renderer::DrawText(const char* text, uint32_t length, float size, const glm::vec2& position, const glm::vec3& color, FontAsset key, bool is_ui, int depth) {
    const Font& font = Assets::GetFont(key);
    
    float x = position.x;
    float y = position.y;

    const float scale = size / font.font_size;

    uint32_t order = depth;
    if (depth != -1) {
        state.main_depth_index = glm::max(state.main_depth_index, order + 1);
    } else {
        order = state.main_depth_index++;
    }

    if (is_ui) {
        order = depth;
        if (depth != -1) {
            state.ui_depth_index = glm::max(state.ui_depth_index, order + 1);
        } else {
            order = state.ui_depth_index++;
        }
    }

    for (uint32_t i = 0; i < length; ++i) {
        const char c = text[i];
        if (c == '\n') {
            y += size;
            x = position.x;
            continue;
        }

        const Glyph& ch = font.glyphs.find(c)->second;

        if (c == ' ') {
            x += (ch.advance >> 6) * scale;
            continue;
        }

        const float xpos = x + ch.bearing.x * scale;
        const float ypos = y - ch.bearing.y * scale;
        const glm::vec2 pos = glm::vec2(xpos, ypos);
        const glm::vec2 size = glm::vec2(ch.size) * scale;

        state.glyph_batch.draw_glyph(pos, size, color, font.texture, ch.texture_coords, ch.tex_size, is_ui, order);

        x += (ch.advance >> 6) * scale;
    }
}

void Renderer::DrawBackground(const BackgroundLayer& layer) {
    const math::Rect aabb = math::Rect::from_top_left(layer.position() - layer.anchor().to_vec2() * layer.size(), layer.size());

    if (layer.nonscale() && !state.nonscale_camera_frustum.intersects(aabb)) return;
    if (!layer.nonscale() && !state.camera_frustum.intersects(aabb)) return;

    layer.is_world() ? state.background_renderer.draw_world_layer(layer) : state.background_renderer.draw_layer(layer);
}

void Renderer::DrawParticle(const glm::vec2& position, const glm::quat& rotation, float scale, Particle::Type type, uint8_t variant, int depth) {
    state.particle_renderer.draw_particle(position, rotation, scale, type, variant, depth);
}

Texture Renderer::CreateTexture(LLGL::TextureType type, LLGL::ImageFormat image_format, uint32_t width, uint32_t height, uint32_t layers, int sampler, const uint8_t* data, bool generate_mip_maps) {
    LLGL::TextureDescriptor texture_desc;
    texture_desc.type = type;
    texture_desc.extent = LLGL::Extent3D(width, height, 1);
    texture_desc.arrayLayers = layers;
    texture_desc.bindFlags = LLGL::BindFlags::Sampled | LLGL::BindFlags::ColorAttachment;
    texture_desc.cpuAccessFlags = 0;
    texture_desc.miscFlags = LLGL::MiscFlags::GenerateMips * generate_mip_maps;

    LLGL::ImageView image_view;
    uint32_t components;

    if (image_format == LLGL::ImageFormat::RGBA || 
        image_format == LLGL::ImageFormat::BGRA ||
        image_format == LLGL::ImageFormat::ARGB ||
        image_format == LLGL::ImageFormat::ABGR
    ) components = 4;
    else if (image_format == LLGL::ImageFormat::RGB) components = 3;
    else if (image_format == LLGL::ImageFormat::RG || image_format == LLGL::ImageFormat::DepthStencil) components = 2;
    else components = 1;

    image_view.format = image_format;
    image_view.dataType = LLGL::DataType::UInt8;
    image_view.data = data;
    image_view.dataSize = width * height * layers * components;

    Texture texture;
    texture.id = state.texture_index++;
    texture.texture = Renderer::Context()->CreateTexture(texture_desc, &image_view);
    texture.sampler = sampler;
    texture.size = glm::uvec2(width, height);

    return texture;
}

LLGL::Shader* Renderer::LoadShader(ShaderPath shader_path, const std::vector<ShaderDef>& shader_defs, const std::vector<LLGL::VertexAttribute>& vertex_attributes) {
    const RenderBackend backend = Renderer::Backend();
    const ShaderType shader_type = shader_path.shader_type;

    const std::string path = backend.AssetFolder() + shader_path.name + shader_type.FileExtension(backend);

    if (!FileExists(path.c_str())) {
        LOG_ERROR("Failed to find shader '%s'", path.c_str());
        return nullptr;
    }

    std::ifstream shader_file;
    shader_file.open(path);

    std::stringstream shader_source_str;
    shader_source_str << shader_file.rdbuf();

    std::string shader_source = shader_source_str.str();

    for (const ShaderDef& shader_def : shader_defs) {
        size_t pos;
        while ((pos = shader_source.find(shader_def.name)) != std::string::npos) {
            shader_source.replace(pos, shader_def.name.length(), shader_def.value);
        }
    }

    shader_file.close();

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

    LLGL::Shader* shader = Renderer::Context()->CreateShader(shader_desc);
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

void Renderer::Terminate() {
    Assets::DestroyShaders();

    state.sprite_batch.terminate();
    state.world_renderer.terminate();
    state.background_renderer.terminate();
    state.particle_renderer.terminate();

    if (state.constant_buffer) state.context->Release(*state.constant_buffer);

    if (state.command_buffer) state.context->Release(*state.command_buffer);
    if (state.swap_chain) state.context->Release(*state.swap_chain);

    Assets::DestroyTextures();
    Assets::DestroySamplers();

    LLGL::RenderSystem::Unload(std::move(state.context));
}

//
// ------------------ Batch ---------------------
//

void RenderBatchSprite::init() {
    const RenderBackend backend = Renderer::Backend();
    const auto& context = Renderer::Context();

    m_buffer = new SpriteInstance[MAX_QUADS];
    m_world_buffer = new SpriteInstance[MAX_QUADS];

    const SpriteVertex vertices[] = {
        SpriteVertex(0.0f, 0.0f),
        SpriteVertex(0.0f, 1.0f),
        SpriteVertex(1.0f, 0.0f),
        SpriteVertex(1.0f, 1.0f),
    };

    m_vertex_buffer = CreateVertexBufferInit(sizeof(vertices), vertices, Assets::GetVertexFormat(VertexFormatAsset::SpriteVertex), "SpriteBatch VertexBuffer");

    m_instance_buffer = CreateVertexBuffer(MAX_QUADS * sizeof(SpriteInstance), Assets::GetVertexFormat(VertexFormatAsset::SpriteInstance), "SpriteBatch InstanceBuffer");
    m_world_instance_buffer = CreateVertexBuffer(MAX_QUADS * sizeof(SpriteInstance), Assets::GetVertexFormat(VertexFormatAsset::SpriteInstance), "SpriteBatchWorld InstanceBuffer");

    {
        LLGL::Buffer* buffers[] = { m_vertex_buffer, m_instance_buffer };
        m_buffer_array = context->CreateBufferArray(2, buffers);
    }
    {
        LLGL::Buffer* buffers[] = { m_vertex_buffer, m_world_instance_buffer };
        m_world_buffer_array = context->CreateBufferArray(2, buffers);
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

    const ShaderPipeline& sprite_shader = Assets::GetShader(ShaderAsset::SpriteShader);

    LLGL::GraphicsPipelineDescriptor pipelineDesc;
    pipelineLayoutDesc.debugName = "SpriteBatch Pipeline";
    pipelineDesc.vertexShader = sprite_shader.vs;
    pipelineDesc.fragmentShader = sprite_shader.ps;
    pipelineDesc.geometryShader = sprite_shader.gs;
    pipelineDesc.pipelineLayout = pipelineLayout;
    pipelineDesc.indexFormat = LLGL::Format::Undefined;
    pipelineDesc.primitiveTopology = LLGL::PrimitiveTopology::TriangleStrip;
    pipelineDesc.renderPass = state.swap_chain->GetRenderPass();
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
        if (report->HasErrors()) LOG_ERROR("%s", report->GetText());
    }
}

void RenderBatchSprite::draw_sprite(const BaseSprite& sprite, const glm::vec4& uv_offset_scale, const tl::optional<Texture>& sprite_texture, RenderLayer layer, bool is_ui, int depth) {
    if (m_sprites.size() >= MAX_QUADS) {
        render();
        begin();
    }

    std::vector<SpriteData>& sprites = layer == RenderLayer::World ? m_world_sprites : m_sprites;
    uint32_t& depth_index = layer == RenderLayer::World ? state.world_depth_index : state.main_depth_index;

    uint32_t order = depth;
    if (depth != -1) {
        depth_index = glm::max(depth_index, order + 1);
    } else {
        order = depth_index++;
    }

    if (is_ui) {
        order = depth;
        if (depth != -1) {
            state.ui_depth_index = glm::max(state.ui_depth_index, order + 1);
        } else {
            order = state.ui_depth_index++;
        }
    }

    sprites.push_back(SpriteData {
        .position = sprite.position(),
        .rotation = sprite.rotation(),
        .size = sprite.size(),
        .offset = sprite.anchor().to_vec2(),
        .uv_offset_scale = uv_offset_scale,
        .color = sprite.color(),
        .outline_color = sprite.outline_color(),
        .outline_thickness = sprite.outline_thickness(),
        .texture = sprite_texture.is_some() ? sprite_texture.get() : Texture(),
        .order = order,
        .is_ui = is_ui,
        .is_nonscalable = sprite.nonscalable()
    });
}

void RenderBatchSprite::render() {
    std::vector<SpriteData>& sprites = m_sprites;

    if (sprites.empty()) return;

    std::sort(
        sprites.begin(),
        sprites.end(),
        [](const SpriteData& a, const SpriteData& b) {
            if (!a.is_ui && b.is_ui) return true;
            if (a.is_ui && !b.is_ui) return false;

            return a.texture.id < b.texture.id;
        }
    );
    
    Texture prev_texture = sprites[0].texture;
    uint32_t sprite_count = 0;
    uint32_t total_sprite_count = 0;
    int vertex_offset = 0;

    for (const SpriteData& sprite_data : sprites) {
        const int prev_texture_id = prev_texture.id;
        const int curr_texture_id = sprite_data.texture.id;

        if (prev_texture_id >= 0 && prev_texture_id != curr_texture_id) {
            m_sprite_flush_queue.push_back(FlushData {
                .texture = prev_texture,
                .offset = vertex_offset,
                .count = sprite_count
            });
            sprite_count = 0;
            vertex_offset = total_sprite_count;
        }

        const uint32_t order = sprite_data.is_ui ? sprite_data.order + state.main_depth_index : sprite_data.order;

        int flags = 0;
        flags |= sprite_data.is_nonscalable << SpriteFlags::Nonscale;
        flags |= sprite_data.is_ui << SpriteFlags::UI;
        flags |= (curr_texture_id >= 0) << SpriteFlags::HasTexture;

        m_buffer_ptr->position = glm::vec3(sprite_data.position, static_cast<float>(order));
        m_buffer_ptr->rotation = sprite_data.rotation;
        m_buffer_ptr->size = sprite_data.size;
        m_buffer_ptr->offset = sprite_data.offset;
        m_buffer_ptr->uv_offset_scale = sprite_data.uv_offset_scale;
        m_buffer_ptr->color = sprite_data.color;
        m_buffer_ptr->outline_color = sprite_data.outline_color;
        m_buffer_ptr->outline_thickness = sprite_data.outline_thickness;
        m_buffer_ptr->flags = flags;
        m_buffer_ptr++;

        sprite_count += 1;
        total_sprite_count += 1;

        prev_texture = sprite_data.texture;
    }

    m_sprite_flush_queue.push_back(FlushData {
        .texture = prev_texture,
        .offset = vertex_offset,
        .count = sprite_count
    });

    flush();
}

void RenderBatchSprite::render_world() {
    std::vector<SpriteData>& sprites = m_world_sprites;

    if (sprites.empty()) return;

    std::sort(
        sprites.begin(),
        sprites.end(),
        [](const SpriteData& a, const SpriteData& b) {
            return a.texture.id < b.texture.id;
        }
    );
    
    Texture prev_texture = sprites[0].texture;
    uint32_t sprite_count = 0;
    uint32_t total_sprite_count = 0;
    int vertex_offset = 0;

    for (const SpriteData& sprite_data : sprites) {
        const int prev_texture_id = prev_texture.id;
        const int curr_texture_id = sprite_data.texture.id;

        if (prev_texture_id >= 0 && prev_texture_id != curr_texture_id) {
            m_world_sprite_flush_queue.push_back(FlushData {
                .texture = prev_texture,
                .offset = vertex_offset,
                .count = sprite_count
            });
            sprite_count = 0;
            vertex_offset = total_sprite_count;
        }

        // The first three are reserved for background, walls and tiles
        const float order = static_cast<float>(3 + sprite_data.order);

        int flags = 1 << SpriteFlags::World;
        flags |= sprite_data.is_nonscalable << SpriteFlags::Nonscale;
        flags |= (curr_texture_id >= 0) << SpriteFlags::HasTexture;

        m_world_buffer_ptr->position = glm::vec3(sprite_data.position, order);
        m_world_buffer_ptr->rotation = sprite_data.rotation;
        m_world_buffer_ptr->size = sprite_data.size;
        m_world_buffer_ptr->offset = sprite_data.offset;
        m_world_buffer_ptr->uv_offset_scale = sprite_data.uv_offset_scale;
        m_world_buffer_ptr->color = sprite_data.color;
        m_world_buffer_ptr->outline_color = sprite_data.outline_color;
        m_world_buffer_ptr->outline_thickness = sprite_data.outline_thickness;
        m_world_buffer_ptr->flags = flags;
        m_world_buffer_ptr++;

        sprite_count += 1;
        total_sprite_count += 1;

        prev_texture = sprite_data.texture;
    }

    m_world_sprite_flush_queue.push_back(FlushData {
        .texture = prev_texture,
        .offset = vertex_offset,
        .count = sprite_count
    });

    flush_world();
}

void RenderBatchSprite::flush() {
    auto* const commands = Renderer::CommandBuffer();

    const ptrdiff_t size = (uint8_t*) m_buffer_ptr - (uint8_t*) m_buffer;
    if (size <= (1 << 16)) {
        commands->UpdateBuffer(*m_instance_buffer, 0, m_buffer, size);
    } else {
        Renderer::Context()->WriteBuffer(*m_instance_buffer, 0, m_buffer, size);
    }
    
    commands->SetVertexBufferArray(*m_buffer_array);

    commands->SetPipelineState(*m_pipeline);
    commands->SetResource(0, *state.constant_buffer);

    for (const FlushData& flush_data : m_sprite_flush_queue) {
        const Texture& t = flush_data.texture.id >= 0 ? flush_data.texture : Assets::GetTexture(TextureAsset::Stub);

        commands->SetResource(1, *t.texture);
        commands->SetResource(2, Assets::GetSampler(t.sampler));
        commands->DrawInstanced(4, 0, flush_data.count, flush_data.offset);
    }
}

void RenderBatchSprite::flush_world() {
    auto* const commands = Renderer::CommandBuffer();

    const ptrdiff_t size = (uint8_t*) m_world_buffer_ptr - (uint8_t*) m_world_buffer;
    if (size <= (1 << 16)) {
        commands->UpdateBuffer(*m_world_instance_buffer, 0, m_world_buffer, size);
    } else {
        Renderer::Context()->WriteBuffer(*m_world_instance_buffer, 0, m_world_buffer, size);
    }
    
    commands->SetVertexBufferArray(*m_world_buffer_array);

    commands->SetPipelineState(*m_pipeline);
    commands->SetResource(0, *state.constant_buffer);

    for (const FlushData& flush_data : m_world_sprite_flush_queue) {
        const Texture& t = flush_data.texture.id >= 0 ? flush_data.texture : Assets::GetTexture(TextureAsset::Stub);

        commands->SetResource(1, *t.texture);
        commands->SetResource(2, Assets::GetSampler(t.sampler));
        commands->DrawInstanced(4, 0, flush_data.count, flush_data.offset);
    }
}

void RenderBatchSprite::begin() {
    m_buffer_ptr = m_buffer;
    m_world_buffer_ptr = m_world_buffer;

    m_sprite_flush_queue.clear();
    m_sprites.clear();

    m_world_sprite_flush_queue.clear();
    m_world_sprites.clear();
}

void RenderBatchSprite::terminate() {
    if (m_vertex_buffer) state.context->Release(*m_vertex_buffer);
    if (m_pipeline) state.context->Release(*m_pipeline);

    delete[] m_buffer;
    delete[] m_world_buffer;
}


void RenderBatchGlyph::init() {
    const RenderBackend backend = Renderer::Backend();
    const auto& context = Renderer::Context();

    m_buffer = new GlyphInstance[MAX_QUADS];

    const GlyphVertex vertices[] = {
        GlyphVertex(0.0f, 0.0f),
        GlyphVertex(0.0f, 1.0f),
        GlyphVertex(1.0f, 0.0f),
        GlyphVertex(1.0f, 1.0f),
    };

    m_vertex_buffer = CreateVertexBufferInit(sizeof(vertices), vertices, Assets::GetVertexFormat(VertexFormatAsset::FontVertex), "GlyphBatch VertexBuffer");
    m_instance_buffer = CreateVertexBuffer(MAX_QUADS * sizeof(GlyphInstance), Assets::GetVertexFormat(VertexFormatAsset::FontInstance), "GlyphBatch InstanceBuffer");

    LLGL::Buffer* buffers[] = { m_vertex_buffer, m_instance_buffer };
    m_buffer_array = context->CreateBufferArray(2, buffers);

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
    pipelineLayoutDesc.debugName = "GlyphBatch Pipeline";
    pipelineDesc.vertexShader = font_shader.vs;
    pipelineDesc.fragmentShader = font_shader.ps;
    pipelineDesc.geometryShader = font_shader.gs;
    pipelineDesc.pipelineLayout = pipelineLayout;
    pipelineDesc.indexFormat = LLGL::Format::R32UInt;
    pipelineDesc.primitiveTopology = LLGL::PrimitiveTopology::TriangleStrip;
    pipelineDesc.renderPass = state.swap_chain->GetRenderPass();
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

void RenderBatchGlyph::draw_glyph(const glm::vec2& pos, const glm::vec2& size, const glm::vec3& color, const Texture& font_texture, const glm::vec2& tex_uv, const glm::vec2& tex_size, bool ui, uint32_t depth) {
    if (m_glyphs.size() >= MAX_QUADS) {
        render();
        begin();
    }

    m_glyphs.push_back(GlyphData {
        .texture = font_texture,
        .color = color,
        .pos = pos,
        .size = size,
        .tex_size = tex_size,
        .tex_uv = tex_uv,
        .order = depth,
        .is_ui = ui
    });
}

void RenderBatchGlyph::render() {
    if (is_empty()) return;

    std::vector<GlyphData> sorted_glyphs = m_glyphs;
    std::sort(
        sorted_glyphs.begin(),
        sorted_glyphs.end(),
        [](const GlyphData& a, const GlyphData& b) {
            return a.texture.id < b.texture.id;
        }
    );
    
    Texture prev_texture = sorted_glyphs[0].texture;
    uint32_t sprite_count = 0;
    uint32_t total_sprite_count = 0;
    int vertex_offset = 0;

    for (const GlyphData& glyph_data : sorted_glyphs) {
        if (prev_texture.id >= 0 && prev_texture.id != glyph_data.texture.id) {
            m_glyphs_flush_queue.push_back(FlushData {
                .texture = prev_texture,
                .offset = vertex_offset,
                .count = sprite_count
            });
            sprite_count = 0;
            vertex_offset = total_sprite_count;
        }

        const float order = glyph_data.is_ui ? glyph_data.order + state.main_depth_index : glyph_data.order;

        m_buffer_ptr->color = glyph_data.color;
        m_buffer_ptr->pos = glm::vec3(glyph_data.pos, static_cast<float>(order));
        m_buffer_ptr->size = glyph_data.size;
        m_buffer_ptr->tex_size = glyph_data.tex_size;
        m_buffer_ptr->uv = glyph_data.tex_uv;
        m_buffer_ptr->is_ui = glyph_data.is_ui;
        m_buffer_ptr++;

        sprite_count += 1;
        total_sprite_count += 1;

        prev_texture = glyph_data.texture;
    }

    m_glyphs_flush_queue.push_back(FlushData {
        .texture = prev_texture,
        .offset = vertex_offset,
        .count = sprite_count
    });

    flush();
}

void RenderBatchGlyph::flush() {
    auto* const commands = Renderer::CommandBuffer();

    const ptrdiff_t size = (uint8_t*) m_buffer_ptr - (uint8_t*) m_buffer;
    if (size <= (1 << 16)) {
        commands->UpdateBuffer(*m_instance_buffer, 0, m_buffer, size);
    } else {
        Renderer::Context()->WriteBuffer(*m_instance_buffer, 0, m_buffer, size);
    }
    
    commands->SetVertexBufferArray(*m_buffer_array);

    commands->SetPipelineState(*m_pipeline);
    commands->SetResource(0, *state.constant_buffer);

    for (const FlushData& flush_data : m_glyphs_flush_queue) {
        const Texture& t = flush_data.texture;

        commands->SetResource(1, *t.texture);
        commands->SetResource(2, Assets::GetSampler(t.sampler));
        commands->DrawInstanced(4, 0, flush_data.count, flush_data.offset);
    }
}

void RenderBatchGlyph::begin() {
    m_buffer_ptr = m_buffer;
    m_glyphs_flush_queue.clear();
    m_glyphs.clear();
}

void RenderBatchGlyph::terminate() {
    if (m_vertex_buffer) state.context->Release(*m_vertex_buffer);
    if (m_pipeline) state.context->Release(*m_pipeline);

    delete[] m_buffer;
}