#include <memory>
#include <LLGL/Utils/TypeNames.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include "renderer/renderer.hpp"
#include "renderer/utils.hpp"
#include "renderer/assets.hpp"
#include "renderer/batch.hpp"
#include "assets.hpp"
#include "utils.hpp"

struct ShaderPipeline {
    LLGL::Shader* vs = nullptr; // Vertex shader
    LLGL::Shader* hs = nullptr; // Hull shader (aka. tessellation control shader)
    LLGL::Shader* ds = nullptr; // Domain shader (aka. tessellation evaluation shader)
    LLGL::Shader* gs = nullptr; // Geometry shader
    LLGL::Shader* ps = nullptr; // Pixel shader (aka. fragment shader)
    LLGL::Shader* cs = nullptr; // Compute shader
    
    void Unload(const LLGL::RenderSystemPtr& context) {
        if (vs != nullptr) context->Release(*vs);
        if (hs != nullptr) context->Release(*hs);
        if (ds != nullptr) context->Release(*ds);
        if (gs != nullptr) context->Release(*gs);
        if (ps != nullptr) context->Release(*ps);
        if (cs != nullptr) context->Release(*cs);
    }
};

struct Shaders {
    ShaderPipeline tilemap_shader;
    ShaderPipeline sprite_shader;
    ShaderPipeline ninepatch_shader;
    ShaderPipeline postprocess_shader;
    ShaderPipeline font_shader;
};

static struct RendererState {
    LLGL::RenderSystemPtr context = nullptr;
    LLGL::SwapChain* swap_chain = nullptr;
    LLGL::CommandBuffer* command_buffer = nullptr;
    std::shared_ptr<CustomSurface> surface = nullptr;
    RenderBatchSprite* sprite_batch = nullptr;
    Shaders shaders;
    LLGL::RenderingLimits limits;
    std::unordered_map<LLGL::SamplerDescriptor, LLGL::Sampler*, SamplerDescriptorHasher, SamplerDescriptorEqual> samplers;
} renderer_state;

static constexpr uint32_t TEXTURES_COUNT = 32;

inline LLGL::Shader* load_vertex_shader(const std::string& name, const LLGL::VertexFormat& vertexFormat) {
    LLGL::ShaderDescriptor shaderDesc;
#if defined(BACKEND_OPENGL)
    std::string path = "assets/shaders/opengl/" + name + ".vert";
    shaderDesc = { LLGL::ShaderType::Vertex, path.c_str() };
#elif defined(BACKEND_D3D11)
    std::string path = "assets/shaders/d3d11/" + name + ".hlsl";
    shaderDesc = { LLGL::ShaderType::Vertex, path.c_str(), "VS", "vs_4_0" };
#elif defined(BACKEND_METAL)
    std::string path = "assets/shaders/metal/" + name + ".metal";
    shaderDesc = { LLGL::ShaderType::Vertex, path.c_str(), "VS", "1.1" };
    shaderDesc.flags |= LLGL::ShaderCompileFlags::DefaultLibrary;
#elif defined(BACKEND_VULKAN)
    std::string path = "assets/shaders/vulkan/" + name + ".vert.spv";
    shaderDesc = { LLGL::ShaderType::Vertex, path.c_str() };
    shaderDesc.sourceType = LLGL::ShaderSourceType::BinaryFile;
#endif

    if (!FileExists(path.c_str())) {
        LLGL::Log::Errorf("Failed to find shader '%s'\n", path.c_str());
        return nullptr;
    }

#if DEBUG
    shaderDesc.flags |= LLGL::ShaderCompileFlags::NoOptimization;
#endif

    shaderDesc.vertex.inputAttribs = vertexFormat.attributes;

    LLGL::Shader* shader = renderer_state.context->CreateShader(shaderDesc);
    if (shader->GetReport() != nullptr && shader->GetReport()->HasErrors()) {
        LLGL::Log::Errorf("Failed to create a shader.\nFile: %s\nError: %s", path.c_str(), shader->GetReport()->GetText());
        return nullptr;
    }

    return shader;
}

inline LLGL::Shader* load_fragment_shader(const std::string& name) {
    LLGL::ShaderDescriptor shaderDesc;

#if defined(BACKEND_OPENGL)
    std::string path = "assets/shaders/opengl/" + name + ".frag";
    shaderDesc = { LLGL::ShaderType::Fragment, path.c_str() };
#elif defined(BACKEND_D3D11)
    std::string path = "assets/shaders/d3d11/" + name + ".hlsl";
    shaderDesc = { LLGL::ShaderType::Fragment, path.c_str(), "PS", "ps_4_0" };
#elif defined(BACKEND_METAL)
    std::string path = "assets/shaders/metal/" + name + ".metal";
    shaderDesc = { LLGL::ShaderType::Fragment, path.c_str(), "PS", "1.1" };
    shaderDesc.flags |= LLGL::ShaderCompileFlags::DefaultLibrary;
#elif defined(BACKEND_VULKAN)
    std::string path = "assets/shaders/vulkan/" + name + ".frag.spv";
    shaderDesc = { LLGL::ShaderType::Fragment, path.c_str() };
    shaderDesc.sourceType = LLGL::ShaderSourceType::BinaryFile;
#endif

#if DEBUG
    shaderDesc.flags |= LLGL::ShaderCompileFlags::NoOptimization;
#endif

    LLGL::Shader* shader = renderer_state.context->CreateShader(shaderDesc);
    if (shader->GetReport() != nullptr && shader->GetReport()->HasErrors()) {
        LLGL::Log::Errorf("Failed to create a shader.\nFile: %s\nError: %s", path.c_str(), shader->GetReport()->GetText());
        return nullptr;
    }

    return shader;
}

bool load_shaders() {
    if ((renderer_state.shaders.sprite_shader.vs = load_vertex_shader("sprite", SpriteVertexFormat())) == nullptr)
        return false;
    if ((renderer_state.shaders.sprite_shader.ps = load_fragment_shader("sprite")) == nullptr)
        return false;

    return true;
}

bool Renderer::Init(GLFWwindow* window, const LLGL::Extent2D& resolution) {
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

#if DEBUG
    rendererDesc.flags      = LLGL::RenderSystemFlags::DebugDevice;
    rendererDesc.debugger   = new LLGL::RenderingDebugger();
#endif

    renderer_state.context = LLGL::RenderSystem::Load(rendererDesc, &report);

    if (report.HasErrors()) {
        LLGL::Log::Errorf("%s", report.GetText());
        return false;
    }

    LLGL::SwapChainDescriptor swapChainDesc;
    swapChainDesc.resolution = resolution;
    swapChainDesc.fullscreen = FULLSCREEN ? true : false;

    renderer_state.surface = std::make_shared<CustomSurface>(window, resolution);

    renderer_state.swap_chain = renderer_state.context->CreateSwapChain(swapChainDesc, renderer_state.surface);
    renderer_state.swap_chain->SetVsyncInterval(VSYNC);

    const auto& info = renderer_state.context->GetRendererInfo();

    LLGL::Log::Printf(
        "Renderer:             %s\n"
        "Device:               %s\n"
        "Vendor:               %s\n"
        "Shading Language:     %s\n"
        "Swap Chain Format:    %s\n"
        "Depth/Stencil Format: %s\n",
        info.rendererName.c_str(),
        info.deviceName.c_str(),
        info.vendorName.c_str(),
        info.shadingLanguageName.c_str(),
        LLGL::ToString(renderer_state.swap_chain->GetColorFormat()),
        LLGL::ToString(renderer_state.swap_chain->GetDepthStencilFormat())
    );
    LLGL::Log::Printf("Extensions:\n");
    for (std::string extension : info.extensionNames) {
        LLGL::Log::Printf("%s\n", extension.c_str());
    }

    renderer_state.limits = renderer_state.context->GetRenderingCaps().limits;

    if (!load_shaders()) return false;

    renderer_state.command_buffer = renderer_state.context->CreateCommandBuffer(LLGL::CommandBufferFlags::ImmediateSubmit);

    renderer_state.sprite_batch = new RenderBatchSprite();
    renderer_state.sprite_batch->init();

    return true;
}

void Renderer::Begin(const Camera& camera) {
    auto sprite_batch = renderer_state.sprite_batch;

    renderer_state.sprite_batch->begin();
    renderer_state.sprite_batch->set_projection_matrix(camera.get_projection_matrix());
    renderer_state.sprite_batch->set_view_matrix(camera.get_view_matrix());
}

void Renderer::Render() {
    const auto commands = Renderer::CommandBuffer();
    const auto swap_chain = Renderer::SwapChain();

    commands->Begin();

    renderer_state.sprite_batch->update_buffers();
    renderer_state.sprite_batch->set_buffers();

    commands->BeginRenderPass(*swap_chain);
    commands->Clear(LLGL::ClearFlags::Color, LLGL::ClearValue(0.0f, 0.0f, 0.0f, 1.0f));
    commands->SetViewport(swap_chain->GetResolution());

    renderer_state.sprite_batch->render();

    commands->EndRenderPass();
    commands->End();
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

    glm::vec2 size = glm::vec2(0.0f);

    if (sprite.texture() != nullptr) {
        size = GetTextureSize(sprite.texture()->texture);
    }

    if (sprite.custom_size().is_some()) {
        size = sprite.custom_size().value();
    }

    size = size * sprite.scale();

    glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(sprite.position(), 0.0f));
    transform *= glm::toMat4(sprite.rotation());
    transform = glm::translate(transform, glm::vec3(-size * sprite.anchor(), 0.));
    transform = glm::scale(transform, glm::vec3(size, 1.0f));

    renderer_state.sprite_batch->draw_sprite(transform, sprite.color(), sprite.outline_color(), sprite.outline_thickness(), sprite.aabb(), uv_offset_scale, sprite.texture(), false);
}

void Renderer::Terminate(void) {
    renderer_state.shaders.tilemap_shader.Unload(renderer_state.context);
    renderer_state.shaders.sprite_shader.Unload(renderer_state.context);
    renderer_state.shaders.ninepatch_shader.Unload(renderer_state.context);
    renderer_state.shaders.postprocess_shader.Unload(renderer_state.context);
    renderer_state.shaders.font_shader.Unload(renderer_state.context);

    renderer_state.context->Release(*renderer_state.command_buffer);

    renderer_state.sprite_batch->terminate();

    renderer_state.context->Release(*renderer_state.swap_chain);
    LLGL::RenderSystem::Unload(std::move(renderer_state.context));
}

const LLGL::RenderSystemPtr& Renderer::Context(void) { return renderer_state.context; }
LLGL::SwapChain* Renderer::SwapChain(void) { return renderer_state.swap_chain; }
LLGL::CommandBuffer* Renderer::CommandBuffer(void) { return renderer_state.command_buffer; }
const std::shared_ptr<CustomSurface>& Renderer::Surface(void) { return renderer_state.surface; }


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

    LLGL::VertexFormat vertex_format = SpriteVertexFormat();
    LLGL::BufferDescriptor bufferDesc = LLGL::VertexBufferDesc(MAX_VERTICES * sizeof(SpriteVertex), vertex_format);
    bufferDesc.debugName = "VertexBuffer";
    m_vertex_buffer = renderer_state.context->CreateBuffer(bufferDesc);

    m_index_buffer = CreateIndexBuffer(indices, LLGL::Format::R32UInt);

    m_constant_buffer = renderer_state.context->CreateBuffer(LLGL::ConstantBufferDesc(sizeof(SpriteUniforms)));

    LLGL::PipelineLayoutDescriptor pipelineLayoutDesc;
    pipelineLayoutDesc.bindings = {
        LLGL::BindingDescriptor(
            "UniformBuffer",
            LLGL::ResourceType::Buffer,
            LLGL::BindFlags::ConstantBuffer,
            LLGL::StageFlags::VertexStage,
            0
        ),
    };

    LLGL::PipelineLayout* pipelineLayout = renderer_state.context->CreatePipelineLayout(pipelineLayoutDesc);

    LLGL::GraphicsPipelineDescriptor pipelineDesc;
    {
        pipelineDesc.vertexShader = renderer_state.shaders.sprite_shader.vs;
        pipelineDesc.fragmentShader = renderer_state.shaders.sprite_shader.ps;
        pipelineDesc.pipelineLayout = pipelineLayout;
        pipelineDesc.indexFormat = LLGL::Format::R32UInt;
        pipelineDesc.primitiveTopology = LLGL::PrimitiveTopology::TriangleList;
        pipelineDesc.renderPass = renderer_state.swap_chain->GetRenderPass();
    }

    m_pipeline = renderer_state.context->CreatePipelineState(pipelineDesc);

    if (const LLGL::Report* report = m_pipeline->GetReport()) {
        if (report->HasErrors()) LLGL::Log::Errorf("%s", report->GetText());
    }

    m_textures = std::vector<const Texture*>(TEXTURES_COUNT, 0);

    // m_resource_heap = renderer_state.context->CreateResourceHeap(LLGL::ResourceHeapDescriptor(pipelineLayout, pipelineLayoutDesc.heapBindings.size()));
}

void RenderBatchSprite::draw_sprite(const glm::mat4& transform, const glm::vec4& color, const glm::vec4& outline_color, float outline_thickness, const math::Rect& aabb, const glm::vec4& uv_offset_scale, const Texture* sprite_texture, bool is_ui) {
    // if (!is_ui && !this->m_camera_frustum.intersects(aabb)) return;
    // if (is_ui && !this->m_ui_frustum.intersects(aabb)) return;

    if (m_index_count >= MAX_INDICES) {
        render();
        begin();
    }

    int texture_index = -1;

    /* if (sprite_texture != nullptr) {
        const uint32_t texture_id = sprite_texture->id();
        
        for (size_t i = 0; i < m_texture_index; i++) {
            if (m_textures[i] == texture_id) {
                texture_index = i;
                break;
            }
        }

        if (texture_index == -1) {
            if (m_texture_index >= render_info.max_texture_slots) {
                render();
                begin();
            }

            texture_index = m_texture_index;
            m_textures[m_texture_index] = texture_id;
            m_texture_index++;
        }
    } */

    m_buffer_ptr->transform_col_0 = transform[0];
    m_buffer_ptr->transform_col_1 = transform[1];
    m_buffer_ptr->transform_col_2 = transform[2];
    m_buffer_ptr->transform_col_3 = transform[3];
    m_buffer_ptr->uv_offset_scale = uv_offset_scale;
    m_buffer_ptr->color = color;
    m_buffer_ptr++;

    m_buffer_ptr->transform_col_0 = transform[0];
    m_buffer_ptr->transform_col_1 = transform[1];
    m_buffer_ptr->transform_col_2 = transform[2];
    m_buffer_ptr->transform_col_3 = transform[3];
    m_buffer_ptr->uv_offset_scale = uv_offset_scale;
    m_buffer_ptr->color = color;
    m_buffer_ptr++;

    m_buffer_ptr->transform_col_0 = transform[0];
    m_buffer_ptr->transform_col_1 = transform[1];
    m_buffer_ptr->transform_col_2 = transform[2];
    m_buffer_ptr->transform_col_3 = transform[3];
    m_buffer_ptr->uv_offset_scale = uv_offset_scale;
    m_buffer_ptr->color = color;
    m_buffer_ptr++;

    m_buffer_ptr->transform_col_0 = transform[0];
    m_buffer_ptr->transform_col_1 = transform[1];
    m_buffer_ptr->transform_col_2 = transform[2];
    m_buffer_ptr->transform_col_3 = transform[3];
    m_buffer_ptr->uv_offset_scale = uv_offset_scale;
    m_buffer_ptr->color = color;
    m_buffer_ptr++;

    m_index_count += 6;
}

void RenderBatchSprite::update_buffers() {
    const auto commands = Renderer::CommandBuffer();

    ptrdiff_t size = (uint8_t*) m_buffer_ptr - (uint8_t*) m_buffer;
    commands->UpdateBuffer(*m_vertex_buffer, 0, m_buffer, size);

    SpriteUniforms uniforms;
    uniforms.view_projection = m_camera_projection * m_camera_view;

    commands->UpdateBuffer(*m_constant_buffer, 0, &uniforms, sizeof(uniforms));
}

void RenderBatchSprite::set_buffers() {
    const auto commands = Renderer::CommandBuffer();

    commands->SetVertexBuffer(*m_vertex_buffer);
    commands->SetIndexBuffer(*m_index_buffer);
}

void RenderBatchSprite::render() {
    if (is_empty()) return;

    const auto commands = Renderer::CommandBuffer();

    commands->SetPipelineState(*m_pipeline);
    commands->SetResource(0, *m_constant_buffer);
    commands->DrawIndexed(m_index_count, 0);

    m_index_count = 0;
}

void RenderBatchSprite::begin(void) {
    m_buffer_ptr = m_buffer;
    m_texture_index = 0;
}

void RenderBatchSprite::terminate() {
    renderer_state.context->Release(*m_vertex_buffer);
    renderer_state.context->Release(*m_index_buffer);
    renderer_state.context->Release(*m_constant_buffer);
    renderer_state.context->Release(*m_pipeline);
    // renderer_state.context->Release(*m_resource_heap);

    delete[] m_buffer;
}