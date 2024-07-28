#include <memory>
#include <array>
#include <LLGL/Utils/TypeNames.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include "renderer/renderer.hpp"
#include "renderer/utils.hpp"
#include "renderer/assets.hpp"
#include "renderer/batch.hpp"
#include "renderer/world_renderer.hpp"
#include "assets.hpp"
#include "utils.hpp"
#include "log.hpp"

static struct RendererState {
    LLGL::RenderSystemPtr context = nullptr;
    LLGL::SwapChain* swap_chain = nullptr;
    LLGL::CommandBuffer* command_buffer = nullptr;
    LLGL::CommandQueue* command_queue = nullptr;
    std::shared_ptr<CustomSurface> surface = nullptr;
    LLGL::RenderingLimits limits;
    
    RenderBatchSprite sprite_batch;
    WorldRenderer world_renderer;
} state;

static constexpr uint32_t MAX_TEXTURES_COUNT = 32;

const LLGL::RenderSystemPtr& Renderer::Context(void) { return state.context; }
LLGL::SwapChain* Renderer::SwapChain(void) { return state.swap_chain; }
LLGL::CommandBuffer* Renderer::CommandBuffer(void) { return state.command_buffer; }
const std::shared_ptr<CustomSurface>& Renderer::Surface(void) { return state.surface; }

bool Renderer::InitEngine() {
    LLGL::Report report;

    void* configPtr = nullptr;
    size_t configSize = 0;

#ifdef BACKEND_OPENGL
    LLGL::RendererConfigurationOpenGL config;
    config.majorVersion = 4;
    config.minorVersion = 3;
    configPtr = &config;
    configSize = sizeof(config);
#endif

    LLGL::RenderSystemDescriptor rendererDesc;
    rendererDesc.moduleName = BACKEND;
    rendererDesc.rendererConfig = configPtr;
    rendererDesc.rendererConfigSize = configSize;

// #if DEBUG
//     rendererDesc.flags      = LLGL::RenderSystemFlags::DebugDevice;
//     rendererDesc.debugger   = new LLGL::RenderingDebugger();
// #endif

    state.context = LLGL::RenderSystem::Load(rendererDesc, &report);

    if (report.HasErrors()) {
        LOG_ERROR("%s", report.GetText());
        return false;
    }

    return true;
}

bool Renderer::Init(GLFWwindow* window, const LLGL::Extent2D& resolution) {
    LLGL::RenderSystemPtr& context = state.context;

    LLGL::SwapChainDescriptor swapChainDesc;
    swapChainDesc.resolution = resolution;
    swapChainDesc.fullscreen = FULLSCREEN ? true : false;

    state.surface = std::make_shared<CustomSurface>(window, resolution);

    state.swap_chain = context->CreateSwapChain(swapChainDesc, state.surface);
    state.swap_chain->SetVsyncInterval(VSYNC);

    const auto& info = context->GetRendererInfo();

    LOG_INFO("Renderer:             %s", info.rendererName.c_str());
    LOG_INFO("Device:               %s", info.deviceName.c_str());
    LOG_INFO("Vendor:               %s", info.vendorName.c_str());
    LOG_INFO("Shading Language:     %s", info.shadingLanguageName.c_str());
    LOG_INFO("Swap Chain Format:    %s", LLGL::ToString(state.swap_chain->GetColorFormat()));
    LOG_INFO("Depth/Stencil Format: %s", LLGL::ToString(state.swap_chain->GetDepthStencilFormat()));

    LOG_INFO("Extensions:");
    for (std::string extension : info.extensionNames) {
        LOG_INFO("  %s", extension.c_str());
    }

    state.limits = context->GetRenderingCaps().limits;

    state.command_buffer = context->CreateCommandBuffer();
    state.command_queue = context->GetCommandQueue();

    state.sprite_batch.init();
    state.world_renderer.Init();

    return true;
}

void Renderer::Begin(const Camera& camera) {
    auto sprite_batch = state.sprite_batch;
    const auto commands = Renderer::CommandBuffer();
    const auto queue = state.command_queue;
    const auto swap_chain = Renderer::SwapChain();

    const glm::vec2 top_left = camera.position() + camera.get_projection_area().min;
    const glm::vec2 bottom_right = camera.position() + camera.get_projection_area().max;

    const math::Rect camera_frustum = math::Rect::from_corners(top_left, bottom_right);
    const math::Rect ui_frustum = math::Rect::from_corners(glm::vec2(0.0), camera.viewport());

    const glm::mat4 screen_projection = glm::ortho(0.0f, static_cast<float>(camera.viewport().x), static_cast<float>(camera.viewport().y), 0.0f, -1.0f, 1.0f);

    commands->Begin();

    commands->BeginRenderPass(*swap_chain);
    commands->Clear(LLGL::ClearFlags::Color, LLGL::ClearValue(0.854f, 0.584f, 0.584f, 1.0f));
    commands->SetViewport(swap_chain->GetResolution());

    state.sprite_batch.begin();
    state.sprite_batch.set_projection_matrix(camera.get_projection_matrix());
    state.sprite_batch.set_view_matrix(camera.get_view_matrix());
    state.sprite_batch.set_screen_projection_matrix(screen_projection);
    state.sprite_batch.set_camera_frustum(camera_frustum);
    state.sprite_batch.set_ui_frustum(ui_frustum);

    state.world_renderer.set_projection_matrix(camera.get_projection_matrix());
    state.world_renderer.set_view_matrix(camera.get_view_matrix());
}

void Renderer::RenderWorld(const World& world) {
    state.world_renderer.Render(world);
}

void Renderer::Render() {
    const auto commands = Renderer::CommandBuffer();
    const auto queue = state.command_queue;
    const auto swap_chain = Renderer::SwapChain();

    state.sprite_batch.render();

    commands->EndRenderPass();
    commands->End();

    queue->Submit(*commands);

    swap_chain->Present();
}

void Renderer::DrawSprite(const Sprite& sprite, RenderLayer render_layer) {
    glm::vec4 uv_offset_scale = glm::vec4(0.0, 0.0, 1.0, 1.0);

    if (sprite.flip_x()) {
        uv_offset_scale.x += uv_offset_scale.z;
        uv_offset_scale.z *= -1.0;
    }

    if (sprite.flip_y()) {
        uv_offset_scale.y += uv_offset_scale.w;
        uv_offset_scale.w *= -1.0;
    }

    state.sprite_batch.draw_sprite(sprite, uv_offset_scale, sprite.texture(), render_layer == RenderLayer::UI);
}

void Renderer::DrawAtlasSprite(const TextureAtlasSprite& sprite, RenderLayer render_layer) {
    const math::Rect& rect = sprite.atlas().rects()[sprite.index()];

    glm::vec4 uv_offset_scale = glm::vec4(
        rect.min.x / sprite.atlas().texture().size().x,
        rect.min.y / sprite.atlas().texture().size().y,
        rect.size().x / sprite.atlas().texture().size().x,
        rect.size().y / sprite.atlas().texture().size().y
    );

    if (sprite.flip_x()) {
        uv_offset_scale.x += uv_offset_scale.z;
        uv_offset_scale.z *= -1.0;
    }

    if (sprite.flip_y()) {
        uv_offset_scale.y += uv_offset_scale.w;
        uv_offset_scale.w *= -1.0;
    }

    state.sprite_batch.draw_sprite(sprite, uv_offset_scale, sprite.atlas().texture(), false);
}

void Renderer::Terminate(void) {
    state.command_queue->WaitIdle();

    Assets::DestroyShaders();

    state.sprite_batch.terminate();
    state.world_renderer.Terminate();

    state.context->Release(*state.command_buffer);
    state.context->Release(*state.swap_chain);

    Assets::DestroyTextures();
    Assets::DestroySamplers();

    LLGL::RenderSystem::Unload(std::move(state.context));
}

void Renderer::FlushSpriteBatch(void) {
    state.sprite_batch.render();
    state.sprite_batch.clear_sprites();
}

void RenderBatchSprite::init() {
    m_buffer = new SpriteVertex[MAX_VERTICES];

    uint32_t indices[MAX_INDICES];
    uint32_t offset = 0;

    for (size_t i = 0; i < MAX_INDICES; i += 6) {
        indices[i + 0] = 0 + offset;
        indices[i + 1] = 1 + offset;
        indices[i + 2] = 2 + offset;

        indices[i + 3] = 2 + offset;
        indices[i + 4] = 3 + offset;
        indices[i + 5] = 1 + offset;

        offset += 4;
    }

    LLGL::BufferDescriptor bufferDesc = LLGL::VertexBufferDesc(MAX_VERTICES * sizeof(SpriteVertex), SpriteVertexFormat());
    bufferDesc.debugName = "SpriteBatch VertexBuffer";
    m_vertex_buffer = state.context->CreateBuffer(bufferDesc);

    m_index_buffer = CreateIndexBuffer(indices, LLGL::Format::R32UInt);

    m_constant_buffer = state.context->CreateBuffer(LLGL::ConstantBufferDesc(sizeof(SpriteUniforms)));


#ifdef BACKEND_OPENGL
    constexpr uint32_t samplerBinding = 1;
#else
    constexpr uint32_t samplerBinding = 2;
#endif

    LLGL::PipelineLayoutDescriptor pipelineLayoutDesc;
    pipelineLayoutDesc.bindings = {
        LLGL::BindingDescriptor(
            "UniformBuffer",
            LLGL::ResourceType::Buffer,
            LLGL::BindFlags::ConstantBuffer,
            LLGL::StageFlags::VertexStage,
            0
        ),
        LLGL::BindingDescriptor("u_texture", LLGL::ResourceType::Texture, LLGL::BindFlags::Sampled, LLGL::StageFlags::FragmentStage, 1),
        LLGL::BindingDescriptor("u_sampler", LLGL::ResourceType::Sampler, 0, LLGL::StageFlags::FragmentStage, samplerBinding),
    };

    LLGL::PipelineLayout* pipelineLayout = state.context->CreatePipelineLayout(pipelineLayoutDesc);

    const ShaderPipeline& sprite_shader = Assets::GetShader(ShaderAssetKey::SpriteShader);

    LLGL::GraphicsPipelineDescriptor pipelineDesc;
    pipelineDesc.vertexShader = sprite_shader.vs;
    pipelineDesc.fragmentShader = sprite_shader.ps;
    pipelineDesc.pipelineLayout = pipelineLayout;
    pipelineDesc.indexFormat = LLGL::Format::R32UInt;
    pipelineDesc.primitiveTopology = LLGL::PrimitiveTopology::TriangleList;
    pipelineDesc.renderPass = state.swap_chain->GetRenderPass();
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

    m_pipeline = state.context->CreatePipelineState(pipelineDesc);

    if (const LLGL::Report* report = m_pipeline->GetReport()) {
        if (report->HasErrors()) LOG_ERROR("%s", report->GetText());
    }
}

void RenderBatchSprite::draw_sprite(const BaseSprite& sprite, const glm::vec4& uv_offset_scale, const tl::optional<Texture>& sprite_texture, bool is_ui) {
    if (!is_ui && !this->m_camera_frustum.intersects(sprite.aabb())) return;
    if (is_ui && !this->m_ui_frustum.intersects(sprite.aabb())) return;

    glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(sprite.position(), 0.0f));
    transform *= glm::toMat4(sprite.rotation());
    transform = glm::translate(transform, glm::vec3(-sprite.size() * sprite.anchor(), 0.));
    transform = glm::scale(transform, glm::vec3(sprite.size(), 1.0f));

    if (m_index_count >= MAX_INDICES) {
        render();
        begin();
    }

    m_sprites.push_back(SpriteData {
        .transform = transform,
        .uv_offset_scale = uv_offset_scale,
        .color = sprite.color(),
        .texture = sprite_texture,
        .index = m_sprite_index++,
        .is_ui = is_ui
    });
}

void RenderBatchSprite::update_buffers() {
    const auto commands = Renderer::CommandBuffer();

    ptrdiff_t size = (uint8_t*) m_buffer_ptr - (uint8_t*) m_buffer;
    commands->UpdateBuffer(*m_vertex_buffer, 0, m_buffer, size);

    SpriteUniforms uniforms;
    uniforms.view_projection = m_camera_projection * m_camera_view;
    uniforms.screen_projection = m_screen_projection;

    commands->UpdateBuffer(*m_constant_buffer, 0, &uniforms, sizeof(uniforms));
}

void RenderBatchSprite::render() {
    if (is_empty()) return;

    std::vector<SpriteData> sorted_sprites = m_sprites;
    std::sort(
        sorted_sprites.begin(),
        sorted_sprites.end(),
        [](const SpriteData& a, const SpriteData& b) {
            int a_id = a.texture.is_some() ? a.texture->id : -1;
            int b_id = b.texture.is_some() ? b.texture->id : -1;

            if (!a.is_ui && b.is_ui) return true;
            if (a.is_ui && !b.is_ui) return false;

            if (a_id < b_id) return true;
            if (b_id < a_id) return false;

            return false;
        }
    );
    
    tl::optional<Texture> prev_texture = sorted_sprites[0].texture;
    size_t vertex_offset = m_sprite_count * 4;

    for (const SpriteData& sprite_data : sorted_sprites) {
        int prev_texture_id = prev_texture.is_some() ? prev_texture->id : -1;
        int curr_texture_id = sprite_data.texture.is_some() ? sprite_data.texture->id : -1;

        if (prev_texture_id >= 0 && prev_texture_id != curr_texture_id) {
            flush(prev_texture, vertex_offset);
            vertex_offset = m_sprite_count * 4;
        }

        m_buffer_ptr->transform_col_0 = sprite_data.transform[0];
        m_buffer_ptr->transform_col_1 = sprite_data.transform[1];
        m_buffer_ptr->transform_col_2 = sprite_data.transform[2];
        m_buffer_ptr->transform_col_3 = sprite_data.transform[3];
        m_buffer_ptr->uv_offset_scale = sprite_data.uv_offset_scale;
        m_buffer_ptr->color = sprite_data.color;
        m_buffer_ptr->has_texture = curr_texture_id >= 0;
        m_buffer_ptr->is_ui = sprite_data.is_ui;
        m_buffer_ptr++;

        m_buffer_ptr->transform_col_0 = sprite_data.transform[0];
        m_buffer_ptr->transform_col_1 = sprite_data.transform[1];
        m_buffer_ptr->transform_col_2 = sprite_data.transform[2];
        m_buffer_ptr->transform_col_3 = sprite_data.transform[3];
        m_buffer_ptr->uv_offset_scale = sprite_data.uv_offset_scale;
        m_buffer_ptr->color = sprite_data.color;
        m_buffer_ptr->has_texture = curr_texture_id >= 0;
        m_buffer_ptr->is_ui = sprite_data.is_ui;
        m_buffer_ptr++;

        m_buffer_ptr->transform_col_0 = sprite_data.transform[0];
        m_buffer_ptr->transform_col_1 = sprite_data.transform[1];
        m_buffer_ptr->transform_col_2 = sprite_data.transform[2];
        m_buffer_ptr->transform_col_3 = sprite_data.transform[3];
        m_buffer_ptr->uv_offset_scale = sprite_data.uv_offset_scale;
        m_buffer_ptr->color = sprite_data.color;
        m_buffer_ptr->has_texture = curr_texture_id >= 0;
        m_buffer_ptr->is_ui = sprite_data.is_ui;
        m_buffer_ptr++;

        m_buffer_ptr->transform_col_0 = sprite_data.transform[0];
        m_buffer_ptr->transform_col_1 = sprite_data.transform[1];
        m_buffer_ptr->transform_col_2 = sprite_data.transform[2];
        m_buffer_ptr->transform_col_3 = sprite_data.transform[3];
        m_buffer_ptr->uv_offset_scale = sprite_data.uv_offset_scale;
        m_buffer_ptr->color = sprite_data.color;
        m_buffer_ptr->has_texture = curr_texture_id >= 0;
        m_buffer_ptr->is_ui = sprite_data.is_ui;
        m_buffer_ptr++;

        m_index_count += 6;
        m_sprite_count += 1;

        prev_texture = sprite_data.texture;
    }

    update_buffers();

    flush(prev_texture, vertex_offset);
}

void RenderBatchSprite::flush(const tl::optional<Texture>& texture, size_t vertex_offset) {
    const auto commands = Renderer::CommandBuffer();
    
    const Texture& t = texture.is_some() ? texture.get() : Assets::GetTexture(AssetKey::TextureStub);

    commands->SetVertexBuffer(*m_vertex_buffer);
    commands->SetIndexBuffer(*m_index_buffer);

    commands->SetPipelineState(*m_pipeline);
    commands->SetResource(0, *m_constant_buffer);
    commands->SetResource(1, *t.texture);
    commands->SetResource(2, Assets::GetSampler(t.sampler));
    commands->DrawIndexed(m_index_count, 0, vertex_offset);

    m_index_count = 0;
}

void RenderBatchSprite::begin(void) {
    m_buffer_ptr = m_buffer;
    m_sprite_index = 0;
    m_sprite_count = 0;
    clear_sprites();
}

void RenderBatchSprite::terminate() {
    state.context->Release(*m_vertex_buffer);
    state.context->Release(*m_index_buffer);
    state.context->Release(*m_constant_buffer);
    state.context->Release(*m_pipeline);

    delete[] m_buffer;
}