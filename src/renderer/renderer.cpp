#include "renderer.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <sstream>
#include <fstream>

#include <LLGL/Utils/TypeNames.h>
#include <LLGL/CommandBufferFlags.h>
#include <LLGL/PipelineStateFlags.h>
#include <LLGL/RendererConfiguration.h>
#include <LLGL/Format.h>
#include <LLGL/RenderPassFlags.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include <tracy/Tracy.hpp>

#include "../assets.hpp"
#include "../log.hpp"
#include "../types/shader_type.hpp"
#include "../utils.hpp"
#include "../time/timer.hpp"
#include "../time/time.hpp"

#include "types.hpp"
#include "utils.hpp"
#include "world_renderer.hpp"
#include "background_renderer.hpp"
#include "particle_renderer.hpp"
#include "macros.hpp"
#include "../ui/utils.hpp"

struct DrawCommandSprite {
    Texture texture;
    glm::quat rotation;
    glm::vec4 uv_offset_scale;
    glm::vec4 color;
    glm::vec4 outline_color;
    glm::vec2 position;
    glm::vec2 size;
    glm::vec2 offset;
    uint32_t order;
    float outline_thickness;
    bool is_ui;
    bool ignore_camera_zoom;
};

struct DrawCommandNinePatch {
    Texture texture;
    glm::quat rotation;
    glm::vec4 uv_offset_scale;
    glm::vec4 color;
    glm::uvec4 margin;
    glm::vec2 position;
    glm::vec2 size;
    glm::vec2 offset;
    glm::vec2 source_size;
    glm::vec2 output_size;
    uint32_t order;
    bool is_ui;
};

struct DrawCommandGlyph {
    Texture texture;
    glm::vec3 color;
    glm::vec2 pos;
    glm::vec2 size;
    glm::vec2 tex_size;
    glm::vec2 tex_uv;
    uint32_t order;
    bool is_ui;
};

class DrawCommand {
public:
    enum Type : uint8_t {
        DrawSprite = 0,
        DrawGlyph,
        DrawNinePatch
    };

    DrawCommand(DrawCommandSprite sprite_data) :
        m_sprite_data(sprite_data),
        m_type(Type::DrawSprite) {}

    DrawCommand(DrawCommandGlyph glyph_data) :
        m_glyph_data(glyph_data),
        m_type(Type::DrawGlyph) {}

    DrawCommand(DrawCommandNinePatch ninepatch_data) :
        m_ninepatch_data(ninepatch_data),
        m_type(Type::DrawNinePatch) {}

    [[nodiscard]] inline Type type() const { return m_type; }

    [[nodiscard]] inline uint32_t order() const {
        switch (m_type) {
        case DrawSprite: return m_sprite_data.order;
        case DrawGlyph: return m_glyph_data.order;
        case DrawNinePatch: return m_ninepatch_data.order;
        }
    }

    [[nodiscard]] inline const Texture& texture() const {
        switch (m_type) {
        case DrawSprite: return m_sprite_data.texture;
        case DrawGlyph: return m_glyph_data.texture;
        case DrawNinePatch: return m_ninepatch_data.texture;
        }
    }

    [[nodiscard]] inline const DrawCommandSprite& sprite_data() const { return m_sprite_data; }
    [[nodiscard]] inline const DrawCommandGlyph& glyph_data() const { return m_glyph_data; }
    [[nodiscard]] inline const DrawCommandNinePatch& ninepatch_data() const { return m_ninepatch_data; }

private:
    union {
        DrawCommandNinePatch m_ninepatch_data;
        DrawCommandSprite m_sprite_data;
        DrawCommandGlyph m_glyph_data;
    };

    Type m_type;
};

enum class FlushDataType : uint8_t {
    Sprite = 0,
    Glyph,
    NinePatch,
};

struct FlushData {
    Texture texture;
    uint32_t offset;
    uint32_t count;
    FlushDataType type;
};

constexpr size_t MAX_QUADS = 5000 / 2;

namespace SpriteFlags {
    enum : uint8_t {
        HasTexture = 0,
        UI,
        World,
        IgnoreCameraZoom,
    };
};

struct SpriteBatchData {
    LLGL::PipelineState* pipeline = nullptr;

    LLGL::Buffer* vertex_buffer = nullptr;
    LLGL::Buffer* instance_buffer = nullptr;
    LLGL::Buffer* world_instance_buffer = nullptr;

    LLGL::BufferArray* buffer_array = nullptr;
    LLGL::BufferArray* world_buffer_array = nullptr;
    
    SpriteInstance* buffer = nullptr;
    SpriteInstance* buffer_ptr = nullptr;
    SpriteInstance* world_buffer = nullptr;
    SpriteInstance* world_buffer_ptr = nullptr;

    uint32_t count = 0;
    uint32_t world_count = 0;

    inline void reset() {
        buffer_ptr = buffer;
        world_buffer_ptr = world_buffer;
        count = 0;
        world_count = 0;
    }
};

struct GlyphBatchData {
    LLGL::PipelineState* pipeline = nullptr;

    LLGL::Buffer* vertex_buffer = nullptr;
    LLGL::Buffer* instance_buffer = nullptr;
    LLGL::BufferArray* buffer_array = nullptr;

    GlyphInstance* buffer = nullptr;
    GlyphInstance* buffer_ptr = nullptr;

    uint32_t count = 0;

    inline void reset() {
        buffer_ptr = buffer;
        count = 0;
    }
};

struct NinePatchBatchData {
    LLGL::Buffer* vertex_buffer = nullptr;
    LLGL::Buffer* instance_buffer = nullptr;
    LLGL::BufferArray* buffer_array = nullptr;

    LLGL::PipelineState* pipeline = nullptr;

    NinePatchInstance* buffer = nullptr;
    NinePatchInstance* buffer_ptr = nullptr;

    uint32_t count = 0;

    inline void reset() {
        buffer_ptr = buffer;
        count = 0;
    }
};

static struct RendererState {
    ParticleRenderer particle_renderer;
    BackgroundRenderer background_renderer;
    WorldRenderer world_renderer;

    std::vector<DrawCommand> draw_commands;
    std::vector<DrawCommand> world_draw_commands;

    std::vector<FlushData> flush_queue;
    std::vector<FlushData> world_flush_queue;

    SpriteBatchData sprite_batch_data;
    GlyphBatchData glyph_batch_data;
    NinePatchBatchData ninepatch_batch_data;

    Timer compute_light_timer;

    LLGL::RenderSystemPtr context = nullptr;
    std::shared_ptr<CustomSurface> surface = nullptr;

    math::Rect ui_frustum;
    math::Rect nozoom_camera_frustum;
    math::Rect camera_frustum;

    LLGL::SwapChain* swap_chain = nullptr;
    LLGL::CommandBuffer* command_buffer = nullptr;
    LLGL::CommandQueue* command_queue = nullptr;
#if DEBUG
    LLGL::RenderingDebugger* debugger = nullptr;
#endif

    LLGL::Buffer* constant_buffer = nullptr;
    LLGL::ResourceHeap* resource_heap = nullptr;

    LLGL::Buffer* chunk_vertex_buffer = nullptr;
    LLGL::RenderTarget* world_render_target = nullptr;
    LLGL::Texture* world_render_texture = nullptr;
    LLGL::Texture* world_depth_texture = nullptr;

    LLGL::Texture* background_render_texture = nullptr;
    LLGL::RenderTarget* background_render_target = nullptr;

    LLGL::Texture* static_lightmap_texture = nullptr;
    LLGL::RenderTarget* static_lightmap_target = nullptr;
    LLGL::RenderPass* static_lightmap_render_pass = nullptr;

    LLGL::PipelineState* postprocess_pipeline = nullptr;
    LLGL::Buffer* fullscreen_triangle_vertex_buffer = nullptr;
    
    Depth depth;

    uint32_t main_depth_index = 0;
    uint32_t world_depth_index = 0;

    uint32_t max_main_depth = 0;
    uint32_t max_world_depth = 0;

    uint32_t texture_index = 0;

    RenderBackend backend;
    bool depth_mode = false;
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
LLGL::Buffer* Renderer::ChunkVertexBuffer() { return state.chunk_vertex_buffer; }

#if DEBUG
LLGL::RenderingDebugger* Renderer::Debugger() { return state.debugger; }
#endif

static void init_sprite_batch() {
    ZoneScopedN("RenderBatchSprite::init");

    const RenderBackend backend = state.backend;
    const auto& context = state.context;

    state.sprite_batch_data.buffer = new SpriteInstance[MAX_QUADS];
    state.sprite_batch_data.world_buffer = new SpriteInstance[MAX_QUADS];

    const Vertex vertices[] = {
        Vertex(0.0f, 0.0f),
        Vertex(0.0f, 1.0f),
        Vertex(1.0f, 0.0f),
        Vertex(1.0f, 1.0f),
    };

    state.sprite_batch_data.vertex_buffer = CreateVertexBufferInit(sizeof(vertices), vertices, Assets::GetVertexFormat(VertexFormatAsset::SpriteVertex), "SpriteBatch VertexBuffer");

    state.sprite_batch_data.instance_buffer = CreateVertexBuffer(MAX_QUADS * sizeof(SpriteInstance), Assets::GetVertexFormat(VertexFormatAsset::SpriteInstance), "SpriteBatch InstanceBuffer");
    state.sprite_batch_data.world_instance_buffer = CreateVertexBuffer(MAX_QUADS * sizeof(SpriteInstance), Assets::GetVertexFormat(VertexFormatAsset::SpriteInstance), "SpriteBatchWorld InstanceBuffer");

    {
        LLGL::Buffer* buffers[] = { state.sprite_batch_data.vertex_buffer, state.sprite_batch_data.instance_buffer };
        state.sprite_batch_data.buffer_array = context->CreateBufferArray(2, buffers);
    }
    {
        LLGL::Buffer* buffers[] = { state.sprite_batch_data.vertex_buffer, state.sprite_batch_data.world_instance_buffer };
        state.sprite_batch_data.world_buffer_array = context->CreateBufferArray(2, buffers);
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
    pipelineDesc.debugName = "SpriteBatch Pipeline";
    pipelineDesc.vertexShader = sprite_shader.vs;
    pipelineDesc.fragmentShader = sprite_shader.ps;
    pipelineDesc.geometryShader = sprite_shader.gs;
    pipelineDesc.pipelineLayout = pipelineLayout;
    pipelineDesc.indexFormat = LLGL::Format::R16UInt;
    pipelineDesc.primitiveTopology = LLGL::PrimitiveTopology::TriangleStrip;
    pipelineDesc.renderPass = state.swap_chain->GetRenderPass();
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

   state.sprite_batch_data.pipeline = context->CreatePipelineState(pipelineDesc);

    if (const LLGL::Report* report = state.sprite_batch_data.pipeline->GetReport()) {
        if (report->HasErrors()) LOG_ERROR("%s", report->GetText());
    }
}

static void draw_sprite(const BaseSprite& sprite, const glm::vec4& uv_offset_scale, const std::optional<Texture>& sprite_texture, bool is_ui, Depth depth) {
    ZoneScopedN("RenderBatchSprite::draw_sprite");

    if (state.depth_mode) depth = state.depth;

    uint32_t& depth_index = state.main_depth_index;
    uint32_t& max_depth = state.max_main_depth;

    const int order = depth.value < 0 ? depth_index : depth.value;
    const uint32_t new_index = depth.value < 0 ? depth_index + 1 : depth.value;

    if (depth.advance) depth_index = std::max(depth_index, static_cast<uint32_t>(new_index));
    if (static_cast<uint32_t>(order) > max_depth) max_depth = order;

    DrawCommandSprite sprite_data = DrawCommandSprite {
        .texture = sprite_texture.has_value() ? sprite_texture.value() : Assets::GetTexture(TextureAsset::Stub),
        .rotation = sprite.rotation(),
        .uv_offset_scale = uv_offset_scale,
        .color = sprite.color(),
        .outline_color = sprite.outline_color(),
        .position = sprite.position(),
        .size = sprite.size(),
        .offset = sprite.anchor().to_vec2(),
        .order = static_cast<uint32_t>(order),
        .outline_thickness = sprite.outline_thickness(),
        .is_ui = is_ui,
        .ignore_camera_zoom = sprite.ignore_camera_zoom()
    };

    state.draw_commands.emplace_back(sprite_data);
    ++state.sprite_batch_data.count;
}

static void draw_world_sprite(const BaseSprite& sprite, const glm::vec4& uv_offset_scale, const std::optional<Texture>& sprite_texture, Depth depth) {
    ZoneScopedN("RenderBatchSprite::draw_world_sprite");

    // if (m_world_sprites.size() >= MAX_QUADS) {
    //     render();
    //     begin();
    // }

    if (state.depth_mode) depth = state.depth;

    uint32_t& depth_index = state.world_depth_index;
    uint32_t& max_depth = state.max_world_depth;

    const int order = depth.value < 0 ? depth_index : depth.value;
    const uint32_t new_index = depth.value < 0 ? depth_index + 1 : depth.value;

    if (depth.advance) depth_index = std::max(depth_index, static_cast<uint32_t>(new_index));
    if (static_cast<uint32_t>(order) > max_depth) max_depth = order;

    DrawCommandSprite sprite_data = DrawCommandSprite {
        .texture = sprite_texture.has_value() ? sprite_texture.value() : Assets::GetTexture(TextureAsset::Stub),
        .rotation = sprite.rotation(),
        .uv_offset_scale = uv_offset_scale,
        .color = sprite.color(),
        .outline_color = sprite.outline_color(),
        .position = sprite.position(),
        .size = sprite.size(),
        .offset = sprite.anchor().to_vec2(),
        .order = static_cast<uint32_t>(order),
        .outline_thickness = sprite.outline_thickness(),
        .is_ui = false,
        .ignore_camera_zoom = sprite.ignore_camera_zoom()
    };

    state.world_draw_commands.emplace_back(sprite_data);
    ++state.sprite_batch_data.world_count;
}

static void init_ninepatch_batch() {
    ZoneScopedN("RenderBatchNinePatch::init");

    const RenderBackend backend = state.backend;
    const auto& context = state.context;

    state.ninepatch_batch_data.buffer = new NinePatchInstance[MAX_QUADS];

    const Vertex vertices[] = {
        Vertex(0.0f, 0.0f),
        Vertex(0.0f, 1.0f),
        Vertex(1.0f, 0.0f),
        Vertex(1.0f, 1.0f),
    };

    state.ninepatch_batch_data.vertex_buffer = CreateVertexBufferInit(sizeof(vertices), vertices, Assets::GetVertexFormat(VertexFormatAsset::NinePatchVertex), "NinePatchBatch VertexBuffer");
    state.ninepatch_batch_data.instance_buffer = CreateVertexBuffer(MAX_QUADS * sizeof(NinePatchInstance), Assets::GetVertexFormat(VertexFormatAsset::NinePatchInstance), "NinePatchBatch InstanceBuffer");

    {
        LLGL::Buffer* buffers[] = { state.ninepatch_batch_data.vertex_buffer, state.ninepatch_batch_data.instance_buffer };
        state.ninepatch_batch_data.buffer_array = context->CreateBufferArray(2, buffers);
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
    pipelineDesc.renderPass = state.swap_chain->GetRenderPass();
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

    state.ninepatch_batch_data.pipeline = context->CreatePipelineState(pipelineDesc);

    if (const LLGL::Report* report = state.ninepatch_batch_data.pipeline->GetReport()) {
        if (report->HasErrors()) LOG_ERROR("%s", report->GetText());
    }
}

static void draw_ninepatch(const NinePatch& ninepatch, const glm::vec4& uv_offset_scale, const Texture& texture, bool is_ui, Depth depth) {
    ZoneScopedN("RenderBatchNinePatch::draw_nine_patch");

    if (state.depth_mode) depth = state.depth;

    uint32_t& depth_index = state.main_depth_index;
    uint32_t& max_depth = state.max_main_depth;

    const int order = depth.value < 0 ? depth_index : depth.value;
    const uint32_t new_index = depth.value < 0 ? depth_index + 1 : depth.value;

    if (depth.advance) depth_index = std::max(depth_index, static_cast<uint32_t>(new_index));
    if (static_cast<uint32_t>(order) > max_depth) max_depth = order;

    DrawCommandNinePatch ninepatch_data = DrawCommandNinePatch {
        .texture = texture,
        .rotation = ninepatch.rotation(),
        .uv_offset_scale = uv_offset_scale,
        .color = ninepatch.color(),
        .margin = ninepatch.margin(),
        .position = ninepatch.position(),
        .size = ninepatch.size(),
        .offset = ninepatch.anchor().to_vec2(),
        .source_size = ninepatch.texture().size(),
        .output_size = ninepatch.size(),
        .order = static_cast<uint32_t>(order),
        .is_ui = is_ui,
    };

    state.draw_commands.emplace_back(ninepatch_data);
    ++state.ninepatch_batch_data.count;
}

static void init_glyph_batch() {
    ZoneScopedN("RenderBatchGlyph::init");

    const RenderBackend backend = state.backend;
    const auto& context = state.context;

    state.glyph_batch_data.buffer = new GlyphInstance[MAX_QUADS];

    const Vertex vertices[] = {
        Vertex(0.0f, 0.0f),
        Vertex(0.0f, 1.0f),
        Vertex(1.0f, 0.0f),
        Vertex(1.0f, 1.0f),
    };

    state.glyph_batch_data.vertex_buffer = CreateVertexBufferInit(sizeof(vertices), vertices, Assets::GetVertexFormat(VertexFormatAsset::FontVertex), "GlyphBatch VertexBuffer");
    state.glyph_batch_data.instance_buffer = CreateVertexBuffer(MAX_QUADS * sizeof(GlyphInstance), Assets::GetVertexFormat(VertexFormatAsset::FontInstance), "GlyphBatch InstanceBuffer");

    LLGL::Buffer* buffers[] = { state.glyph_batch_data.vertex_buffer, state.glyph_batch_data.instance_buffer };
    state.glyph_batch_data.buffer_array = context->CreateBufferArray(2, buffers);

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
    pipelineDesc.renderPass = state.swap_chain->GetRenderPass();
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

    state.glyph_batch_data.pipeline = context->CreatePipelineState(pipelineDesc);

    if (const LLGL::Report* report = state.glyph_batch_data.pipeline->GetReport()) {
        if (report->HasErrors()) LOG_ERROR("%s", report->GetText());
    }
}

static void draw_glyph(const glm::vec2& pos, const glm::vec2& size, const glm::vec3& color, const Texture& font_texture, const glm::vec2& tex_uv, const glm::vec2& tex_size, bool ui, uint32_t depth) {
    ZoneScopedN("RenderBatchGlyph::draw_glyph");

    DrawCommandGlyph glyph_data = DrawCommandGlyph {
        .texture = font_texture,
        .color = color,
        .pos = pos,
        .size = size,
        .tex_size = tex_size,
        .tex_uv = tex_uv,
        .order = depth,
        .is_ui = ui
    };

    state.draw_commands.emplace_back(glyph_data);
    ++state.glyph_batch_data.count;
}

bool Renderer::InitEngine(RenderBackend backend) {
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
    state.debugger = new LLGL::RenderingDebugger();
    rendererDesc.flags    = LLGL::RenderSystemFlags::DebugDevice;
    rendererDesc.debugger = state.debugger;
#endif

    state.context = LLGL::RenderSystem::Load(rendererDesc, &report);
    state.backend = backend;

    if (backend.IsOpenGL()) {
        delete (LLGL::OpenGLContextProfile*) rendererDesc.rendererConfig;
    }

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
    for (const auto& extension : info.extensionNames) {
        LOG_INFO("  %s", extension.c_str());
    }

    return true;
}

bool Renderer::Init(GLFWwindow* window, const LLGL::Extent2D& resolution, bool vsync, bool fullscreen) {
    const LLGL::RenderSystemPtr& context = state.context;

    state.surface = std::make_shared<CustomSurface>(window, resolution);

    LLGL::SwapChainDescriptor swapChainDesc;
    swapChainDesc.resolution = resolution;
    swapChainDesc.fullscreen = fullscreen;

    state.swap_chain = context->CreateSwapChain(swapChainDesc, state.surface);
    state.swap_chain->SetVsyncInterval(vsync);

    LLGL::CommandBufferDescriptor command_buffer_desc;
    command_buffer_desc.numNativeBuffers = 3;

    state.command_buffer = context->CreateCommandBuffer(command_buffer_desc);
    state.command_queue = context->GetCommandQueue();

    state.constant_buffer = CreateConstantBuffer(sizeof(ProjectionsUniform), "ConstantBuffer");

    ResizeTextures(resolution);

    init_sprite_batch();
    init_glyph_batch();
    init_ninepatch_batch();

    const glm::vec2 tile_tex_size = glm::vec2(Assets::GetTexture(TextureAsset::Tiles).size());
    const glm::vec2 wall_tex_size = glm::vec2(Assets::GetTexture(TextureAsset::Walls).size());

    const glm::vec2 tile_padding = glm::vec2(Constants::TILE_TEXTURE_PADDING) / tile_tex_size;
    const glm::vec2 wall_padding = glm::vec2(Constants::WALL_TEXTURE_PADDING) / wall_tex_size;

    const ChunkVertex vertices[] = {
        ChunkVertex(0.0, 0.0, wall_tex_size, tile_tex_size, wall_padding, tile_padding),
        ChunkVertex(0.0, 1.0, wall_tex_size, tile_tex_size, wall_padding, tile_padding),
        ChunkVertex(1.0, 0.0, wall_tex_size, tile_tex_size, wall_padding, tile_padding),
        ChunkVertex(1.0, 1.0, wall_tex_size, tile_tex_size, wall_padding, tile_padding),
    };

    state.chunk_vertex_buffer = CreateVertexBufferInit(sizeof(vertices), vertices, Assets::GetVertexFormat(VertexFormatAsset::TilemapVertex), "WorldRenderer VertexBuffer");

    {
        LLGL::TextureDescriptor lightmap_texture_desc;
        lightmap_texture_desc.type      = LLGL::TextureType::Texture2D;
        lightmap_texture_desc.format    = LLGL::Format::RGBA8UNorm;
        lightmap_texture_desc.extent    = LLGL::Extent3D(resolution.width, resolution.height, 1);
        lightmap_texture_desc.miscFlags = 0;
        lightmap_texture_desc.bindFlags = LLGL::BindFlags::Sampled | LLGL::BindFlags::ColorAttachment;
        lightmap_texture_desc.mipLevels = 1;

        state.static_lightmap_texture = context->CreateTexture(lightmap_texture_desc);

        LLGL::RenderTargetDescriptor render_target;
        render_target.resolution = LLGL::Extent2D(resolution.width, resolution.height);
        render_target.colorAttachments[0].texture = state.static_lightmap_texture;
        state.static_lightmap_target = context->CreateRenderTarget(render_target);

        LLGL::RenderPassDescriptor static_lightmap_render_pass_desc;
        static_lightmap_render_pass_desc.colorAttachments[0].loadOp = LLGL::AttachmentLoadOp::Undefined;
        static_lightmap_render_pass_desc.colorAttachments[0].storeOp = LLGL::AttachmentStoreOp::Store;
        static_lightmap_render_pass_desc.colorAttachments[0].format = lightmap_texture_desc.format;
        state.static_lightmap_render_pass = context->CreateRenderPass(static_lightmap_render_pass_desc);
    }

    LLGL::PipelineLayoutDescriptor pipelineLayoutDesc;
    pipelineLayoutDesc.staticSamplers = {   
        LLGL::StaticSamplerDescriptor("u_background_sampler", LLGL::StageFlags::FragmentStage, 4, Assets::GetSampler(TextureSampler::Nearest).descriptor()),
        LLGL::StaticSamplerDescriptor("u_world_sampler", LLGL::StageFlags::FragmentStage, 6, Assets::GetSampler(TextureSampler::Nearest).descriptor()),
        LLGL::StaticSamplerDescriptor("u_lightmap_sampler", LLGL::StageFlags::FragmentStage, 8, Assets::GetSampler(TextureSampler::Nearest).descriptor()),
        LLGL::StaticSamplerDescriptor("u_light_sampler", LLGL::StageFlags::FragmentStage, 10, Assets::GetSampler(TextureSampler::Nearest).descriptor()),
    };
    pipelineLayoutDesc.combinedTextureSamplers =
    {
        LLGL::CombinedTextureSamplerDescriptor{ "u_background_texture", "u_background_texture", "u_background_sampler", 3 },
        LLGL::CombinedTextureSamplerDescriptor{ "u_world_texture", "u_world_texture", "u_world_sampler", 5 },
        LLGL::CombinedTextureSamplerDescriptor{ "u_lightmap_texture", "u_lightmap_texture", "u_lightmap_sampler", 7 },
        LLGL::CombinedTextureSamplerDescriptor{ "u_light_texture", "u_light_texture", "u_light_sampler", 9 },
    };
    pipelineLayoutDesc.heapBindings = {
        LLGL::BindingDescriptor(
            "GlobalUniformBuffer",
            LLGL::ResourceType::Buffer,
            LLGL::BindFlags::ConstantBuffer,
            LLGL::StageFlags::VertexStage,
            LLGL::BindingSlot(2)
        ),

        LLGL::BindingDescriptor("u_background_texture", LLGL::ResourceType::Texture, LLGL::BindFlags::Sampled, LLGL::StageFlags::FragmentStage, 3),
        LLGL::BindingDescriptor("u_world_texture", LLGL::ResourceType::Texture, LLGL::BindFlags::Sampled, LLGL::StageFlags::FragmentStage, 5),
        LLGL::BindingDescriptor("u_lightmap_texture", LLGL::ResourceType::Texture, LLGL::BindFlags::Sampled, LLGL::StageFlags::FragmentStage, 7),
        LLGL::BindingDescriptor("u_light_texture", LLGL::ResourceType::Texture, LLGL::BindFlags::Sampled, LLGL::StageFlags::FragmentStage, 9),
    };

    LLGL::PipelineLayout* pipelineLayout = context->CreatePipelineLayout(pipelineLayoutDesc);

    const LLGL::ResourceViewDescriptor resource_views[] = {
        state.constant_buffer,
        state.background_render_texture,
        state.world_render_texture,
        state.static_lightmap_texture,
        nullptr,
    };
    state.resource_heap = context->CreateResourceHeap(LLGL::ResourceHeapDescriptor(pipelineLayout, ARRAY_LEN(resource_views)), resource_views);

    const ShaderPipeline& postprocess_shader = Assets::GetShader(ShaderAsset::PostProcessShader);

    LLGL::GraphicsPipelineDescriptor pipelineDesc;
    pipelineDesc.debugName = "LightMap Pipeline";
    pipelineDesc.vertexShader = postprocess_shader.vs;
    pipelineDesc.fragmentShader = postprocess_shader.ps;
    pipelineDesc.pipelineLayout = pipelineLayout;
    pipelineDesc.indexFormat = LLGL::Format::R16UInt;
    pipelineDesc.primitiveTopology = LLGL::PrimitiveTopology::TriangleStrip;
    pipelineDesc.rasterizer.frontCCW = true;

    state.postprocess_pipeline = context->CreatePipelineState(pipelineDesc);

    state.compute_light_timer = Timer::from_seconds(Constants::FIXED_UPDATE_INTERVAL, TimerMode::Repeating);

    state.world_renderer.init(state.static_lightmap_render_pass);
    state.background_renderer.init();
    state.particle_renderer.init();

    return true;
}

void Renderer::ResizeTextures(LLGL::Extent2D resolution) {
    const LLGL::RenderSystemPtr& context = state.context;
    const auto* swap_chain = state.swap_chain;

    RESOURCE_RELEASE(state.world_render_texture);
    RESOURCE_RELEASE(state.world_depth_texture);
    RESOURCE_RELEASE(state.world_render_target);

    RESOURCE_RELEASE(state.background_render_texture);
    RESOURCE_RELEASE(state.background_render_target);

    RESOURCE_RELEASE(state.static_lightmap_texture);
    RESOURCE_RELEASE(state.static_lightmap_target);

    LLGL::TextureDescriptor texture_desc;
    texture_desc.extent = LLGL::Extent3D(resolution.width, resolution.height, 1);
    texture_desc.format = swap_chain->GetColorFormat();
    texture_desc.bindFlags = LLGL::BindFlags::Sampled | LLGL::BindFlags::ColorAttachment;
    texture_desc.miscFlags = 0;
    texture_desc.cpuAccessFlags = 0;
    texture_desc.mipLevels = 1;

    LLGL::TextureDescriptor depth_texture_desc;
    depth_texture_desc.extent = LLGL::Extent3D(resolution.width, resolution.height, 1);
    depth_texture_desc.format = swap_chain->GetDepthStencilFormat();
    depth_texture_desc.bindFlags = LLGL::BindFlags::Sampled | LLGL::BindFlags::DepthStencilAttachment;
    depth_texture_desc.miscFlags = 0;
    depth_texture_desc.cpuAccessFlags = 0;
    depth_texture_desc.mipLevels = 1;

    LLGL::Texture* world_depth_texture = context->CreateTexture(depth_texture_desc);
    LLGL::Texture* world_render_texture = context->CreateTexture(texture_desc);
    LLGL::Texture* background_render_texture = context->CreateTexture(texture_desc);
    LLGL::Texture* static_lightmap_texture = context->CreateTexture(texture_desc);

    state.world_depth_texture = world_depth_texture;
    state.world_render_texture = world_render_texture;
    state.background_render_texture = background_render_texture;
    state.static_lightmap_texture = static_lightmap_texture;
    
    LLGL::RenderTargetDescriptor render_target_desc;
    render_target_desc.resolution = resolution;
    render_target_desc.colorAttachments[0] = world_render_texture;
    render_target_desc.depthStencilAttachment = world_depth_texture;
    render_target_desc.depthStencilAttachment.format = swap_chain->GetDepthStencilFormat();
    state.world_render_target = context->CreateRenderTarget(render_target_desc);
    
    LLGL::RenderTargetDescriptor background_target_desc;
    background_target_desc.resolution = resolution;
    background_target_desc.colorAttachments[0] = background_render_texture;
    background_target_desc.depthStencilAttachment.format = swap_chain->GetDepthStencilFormat();
    state.background_render_target = context->CreateRenderTarget(background_target_desc);

    LLGL::RenderTargetDescriptor static_lightmap_target_desc;
    static_lightmap_target_desc.renderPass = state.static_lightmap_render_pass;
    static_lightmap_target_desc.resolution = resolution;
    static_lightmap_target_desc.colorAttachments[0] = static_lightmap_texture;
    state.static_lightmap_target = context->CreateRenderTarget(static_lightmap_target_desc);

    if (state.resource_heap != nullptr) {
        context->WriteResourceHeap(*state.resource_heap, 1, {background_render_texture});
        context->WriteResourceHeap(*state.resource_heap, 2, {world_render_texture});
        context->WriteResourceHeap(*state.resource_heap, 3, {static_lightmap_texture});
    }
}

void Renderer::InitWorldRenderer(const WorldData &world) {
    using Constants::SUBDIVISION;
    using Constants::TILE_SIZE;

    const auto& context = state.context;

    RESOURCE_RELEASE(state.fullscreen_triangle_vertex_buffer);

    const glm::vec2 world_size = glm::vec2(world.area.size()) * TILE_SIZE;

    const glm::vec2 vertices[] = {
        glm::vec2(-1.0f, 1.0f),  glm::vec2(0.0f, 0.0f), world_size,
        glm::vec2(3.0f,  1.0f),  glm::vec2(2.0f, 0.0f), world_size,
        glm::vec2(-1.0f, -3.0f), glm::vec2(0.0f, 2.0f), world_size,
    };
    state.fullscreen_triangle_vertex_buffer = CreateVertexBufferInit(sizeof(vertices), vertices, Assets::GetVertexFormat(VertexFormatAsset::PostProcessVertex));

    state.world_renderer.init_textures(world);
    context->WriteResourceHeap(*state.resource_heap, 4, {state.world_renderer.light_texture()});

    state.world_renderer.init_lightmap_chunks(world);
}

void Renderer::Begin(const Camera& camera, WorldData& world) {
    ZoneScopedN("Renderer::Begin");

    auto* const commands = state.command_buffer;

    const math::Rect camera_frustum = math::Rect::from_corners(
        camera.position() + camera.get_projection_area().min,
        camera.position() + camera.get_projection_area().max
    );
    const math::Rect nozoom_camera_frustum = math::Rect::from_corners(
        camera.position() + camera.get_nozoom_projection_area().min,
        camera.position() + camera.get_nozoom_projection_area().max
    );
    const math::Rect ui_frustum = math::Rect::from_corners(glm::vec2(0.0), camera.viewport());

    state.camera_frustum = camera_frustum;
    state.nozoom_camera_frustum = nozoom_camera_frustum;
    state.ui_frustum = ui_frustum;

    state.world_renderer.update_lightmap_texture(world);
    state.world_renderer.update_tile_texture(world);

    commands->Begin();
    commands->SetViewport(state.swap_chain->GetResolution());

    state.sprite_batch_data.reset();
    state.glyph_batch_data.reset();
    state.ninepatch_batch_data.reset();

    state.main_depth_index = 0;
    // The first three are reserved for background, walls and tiles
    state.world_depth_index = 3;
    state.max_world_depth = 3;
    state.max_main_depth = 0;

    state.draw_commands.clear();
    state.world_draw_commands.clear();
    state.flush_queue.clear();
    state.world_flush_queue.clear();
}

static FORCE_INLINE void UpdateBuffer(LLGL::Buffer* buffer, void* data, size_t length) {
    if (length < (1 << 16)) {
        state.command_buffer->UpdateBuffer(*buffer, 0, data, length);
    } else {
        state.context->WriteBuffer(*buffer, 0, data, length);
    }
}

static INLINE void SortDrawCommands() {
    ZoneScopedN("Renderer::SortDrawCommands");

    std::sort(
        state.draw_commands.begin(),
        state.draw_commands.end(),
        [](const DrawCommand& a, const DrawCommand& b) {
            const uint32_t a_order = a.order();
            const uint32_t b_order = b.order();

            if (a_order < b_order) return true;
            if (a_order > b_order) return false;

            return a.texture().id() < b.texture().id();
        }
    );

    Texture sprite_prev_texture;
    uint32_t sprite_count = 0;
    uint32_t sprite_total_count = 0;
    uint32_t sprite_vertex_offset = 0;
    uint32_t sprite_remaining = state.sprite_batch_data.count;

    Texture glyph_prev_texture;
    uint32_t glyph_count = 0;
    uint32_t glyph_total_count = 0;
    uint32_t glyph_vertex_offset = 0;
    uint32_t glyph_remaining = state.glyph_batch_data.count;

    Texture ninepatch_prev_texture;
    uint32_t ninepatch_count = 0;
    uint32_t ninepatch_total_count = 0;
    uint32_t ninepatch_vertex_offset = 0;
    uint32_t ninepatch_remaining = state.ninepatch_batch_data.count;

    uint32_t prev_order = state.draw_commands[0].order();

    for (const DrawCommand& draw_command : state.draw_commands) {
        uint32_t new_order = prev_order;

        switch (draw_command.type()) {
        case DrawCommand::DrawSprite: {
            const DrawCommandSprite& sprite_data = draw_command.sprite_data();

            if (sprite_total_count == 0) {
                sprite_prev_texture = sprite_data.texture;
            }

            const uint32_t prev_texture_id = sprite_prev_texture.id();
            const uint32_t curr_texture_id = sprite_data.texture.id();

            if (sprite_count > 0 && prev_texture_id != curr_texture_id) {
                state.flush_queue.push_back(FlushData {
                    .texture = sprite_prev_texture,
                    .offset = sprite_vertex_offset,
                    .count = sprite_count,
                    .type = FlushDataType::Sprite
                });
                sprite_count = 0;
                sprite_vertex_offset = sprite_total_count;
            }

            int flags = 0;
            flags |= sprite_data.ignore_camera_zoom << SpriteFlags::IgnoreCameraZoom;
            flags |= sprite_data.is_ui << SpriteFlags::UI;
            flags |= (curr_texture_id >= 0) << SpriteFlags::HasTexture;

            state.sprite_batch_data.buffer_ptr->position = sprite_data.position;
            state.sprite_batch_data.buffer_ptr->rotation = sprite_data.rotation;
            state.sprite_batch_data.buffer_ptr->size = sprite_data.size;
            state.sprite_batch_data.buffer_ptr->offset = sprite_data.offset;
            state.sprite_batch_data.buffer_ptr->uv_offset_scale = sprite_data.uv_offset_scale;
            state.sprite_batch_data.buffer_ptr->color = sprite_data.color;
            state.sprite_batch_data.buffer_ptr->outline_color = sprite_data.outline_color;
            state.sprite_batch_data.buffer_ptr->outline_thickness = sprite_data.outline_thickness;
            state.sprite_batch_data.buffer_ptr->flags = flags;
            state.sprite_batch_data.buffer_ptr++;

            ++sprite_count;
            ++sprite_total_count;
            --sprite_remaining;

            if (sprite_remaining == 0) {
                state.flush_queue.push_back(FlushData {
                    .texture = sprite_data.texture,
                    .offset = sprite_vertex_offset,
                    .count = sprite_count,
                    .type = FlushDataType::Sprite
                });
                sprite_count = 0;
            }

            sprite_prev_texture = sprite_data.texture;
            new_order = sprite_data.order;
        } break;
        case DrawCommand::DrawGlyph: {
            const DrawCommandGlyph& glyph_data = draw_command.glyph_data();

            if (glyph_total_count == 0) {
                glyph_prev_texture = glyph_data.texture;
            }

            if (glyph_count > 0 && glyph_prev_texture.id() != glyph_data.texture.id()) {
                state.flush_queue.push_back(FlushData {
                    .texture = glyph_prev_texture,
                    .offset = glyph_vertex_offset,
                    .count = glyph_count,
                    .type = FlushDataType::Glyph
                });
                glyph_count = 0;
                glyph_vertex_offset = glyph_total_count;
            }

            state.glyph_batch_data.buffer_ptr->color = glyph_data.color;
            state.glyph_batch_data.buffer_ptr->pos = glyph_data.pos;
            state.glyph_batch_data.buffer_ptr->size = glyph_data.size;
            state.glyph_batch_data.buffer_ptr->tex_size = glyph_data.tex_size;
            state.glyph_batch_data.buffer_ptr->uv = glyph_data.tex_uv;
            state.glyph_batch_data.buffer_ptr->is_ui = glyph_data.is_ui;
            state.glyph_batch_data.buffer_ptr++;

            ++glyph_count;
            ++glyph_total_count;
            --glyph_remaining;

            if (glyph_remaining == 0) {
                state.flush_queue.push_back(FlushData {
                    .texture = glyph_data.texture,
                    .offset = glyph_vertex_offset,
                    .count = glyph_count,
                    .type = FlushDataType::Glyph
                });
                glyph_count = 0;
            }

            glyph_prev_texture = glyph_data.texture;
            new_order = glyph_data.order;
        } break;
        case DrawCommand::DrawNinePatch: {
            const DrawCommandNinePatch& ninepatch_data = draw_command.ninepatch_data();

            if (ninepatch_total_count == 0) {
                ninepatch_prev_texture = ninepatch_data.texture;
            }

            const uint32_t prev_texture_id = ninepatch_prev_texture.id();
            const uint32_t curr_texture_id = ninepatch_data.texture.id();

            if (ninepatch_count > 0 && prev_texture_id != curr_texture_id) {
                state.flush_queue.push_back(FlushData {
                    .texture = ninepatch_prev_texture,
                    .offset = ninepatch_vertex_offset,
                    .count = ninepatch_count,
                    .type = FlushDataType::NinePatch
                });
                ninepatch_count = 0;
                ninepatch_vertex_offset = ninepatch_total_count;
            }

            int flags = ninepatch_data.is_ui << SpriteFlags::UI;

            state.ninepatch_batch_data.buffer_ptr->position = ninepatch_data.position;
            state.ninepatch_batch_data.buffer_ptr->rotation = ninepatch_data.rotation;
            state.ninepatch_batch_data.buffer_ptr->margin = ninepatch_data.margin;
            state.ninepatch_batch_data.buffer_ptr->size = ninepatch_data.size;
            state.ninepatch_batch_data.buffer_ptr->offset = ninepatch_data.offset;
            state.ninepatch_batch_data.buffer_ptr->source_size = ninepatch_data.source_size;
            state.ninepatch_batch_data.buffer_ptr->output_size = ninepatch_data.output_size;
            state.ninepatch_batch_data.buffer_ptr->uv_offset_scale = ninepatch_data.uv_offset_scale;
            state.ninepatch_batch_data.buffer_ptr->color = ninepatch_data.color;
            state.ninepatch_batch_data.buffer_ptr->flags = flags;
            state.ninepatch_batch_data.buffer_ptr++;

            ++ninepatch_count;
            ++ninepatch_total_count;
            --ninepatch_remaining;

            if (ninepatch_remaining == 0) {
                state.flush_queue.push_back(FlushData {
                    .texture = ninepatch_data.texture,
                    .offset = ninepatch_vertex_offset,
                    .count = ninepatch_count,
                    .type = FlushDataType::NinePatch
                });
                ninepatch_count = 0;
            }

            ninepatch_prev_texture = ninepatch_data.texture;
            new_order = ninepatch_data.order;
        } break;
        }

        if (prev_order != new_order) {
            if (sprite_count > 1) {
                state.flush_queue.push_back(FlushData {
                    .texture = sprite_prev_texture,
                    .offset = sprite_vertex_offset,
                    .count = sprite_count,
                    .type = FlushDataType::Sprite
                });
                sprite_count = 0;
                sprite_vertex_offset = sprite_total_count;
            }

            if (glyph_count > 1) {
                state.flush_queue.push_back(FlushData {
                    .texture = glyph_prev_texture,
                    .offset = glyph_vertex_offset,
                    .count = glyph_count,
                    .type = FlushDataType::Glyph
                });
                glyph_count = 0;
                glyph_vertex_offset = glyph_total_count;
            }

            if (ninepatch_count > 1) {
                state.flush_queue.push_back(FlushData {
                    .texture = ninepatch_prev_texture,
                    .offset = ninepatch_vertex_offset,
                    .count = ninepatch_count,
                    .type = FlushDataType::NinePatch
                });
                ninepatch_count = 0;
                ninepatch_vertex_offset = ninepatch_total_count;
            }
        }

        prev_order = new_order;
    }
    
    if (sprite_total_count > 0) {
        const size_t size = (size_t) state.sprite_batch_data.buffer_ptr - (size_t) state.sprite_batch_data.buffer;
        UpdateBuffer(state.sprite_batch_data.instance_buffer, state.sprite_batch_data.buffer, size);
    }

    if (glyph_total_count > 0) {
        const size_t size = (size_t) state.glyph_batch_data.buffer_ptr - (size_t) state.glyph_batch_data.buffer;
        UpdateBuffer(state.glyph_batch_data.instance_buffer, state.glyph_batch_data.buffer, size);
    }

    if (ninepatch_total_count > 0) {
        const size_t size = (size_t) state.ninepatch_batch_data.buffer_ptr - (size_t) state.ninepatch_batch_data.buffer;
        UpdateBuffer(state.ninepatch_batch_data.instance_buffer, state.ninepatch_batch_data.buffer, size);
    }
}

static INLINE void SortWorldDrawCommands() {
    ZoneScopedN("Renderer::SortWorldDrawCommands");

    std::sort(
        state.world_draw_commands.begin(),
        state.world_draw_commands.end(),
        [](const DrawCommand& a, const DrawCommand& b) {
            return a.order() < b.order();
        }
    );

    Texture sprite_prev_texture;
    uint32_t sprite_count = 0;
    uint32_t sprite_total_count = 0;
    uint32_t sprite_vertex_offset = 0;

    for (const DrawCommand& draw_command : state.world_draw_commands) {
        if (draw_command.type() == DrawCommand::DrawSprite) {
            const DrawCommandSprite& sprite_data = draw_command.sprite_data();

            if (sprite_prev_texture.id() < 0) {
                sprite_prev_texture = sprite_data.texture;
            }

            const uint32_t prev_texture_id = sprite_prev_texture.id();
            const uint32_t curr_texture_id = sprite_data.texture.id();

            if (prev_texture_id != curr_texture_id) {
                state.world_flush_queue.push_back(FlushData {
                    .texture = sprite_prev_texture,
                    .offset = sprite_vertex_offset,
                    .count = sprite_count,
                    .type = FlushDataType::Sprite
                });
                sprite_count = 0;
                sprite_vertex_offset = sprite_total_count;
            }

            const float order = static_cast<float>(sprite_data.order);

            int flags = 0;
            flags |= sprite_data.ignore_camera_zoom << SpriteFlags::IgnoreCameraZoom;
            flags |= sprite_data.is_ui << SpriteFlags::UI;
            flags |= (curr_texture_id >= 0) << SpriteFlags::HasTexture;

            state.sprite_batch_data.world_buffer_ptr->position = glm::vec3(sprite_data.position, order);
            state.sprite_batch_data.world_buffer_ptr->rotation = sprite_data.rotation;
            state.sprite_batch_data.world_buffer_ptr->size = sprite_data.size;
            state.sprite_batch_data.world_buffer_ptr->offset = sprite_data.offset;
            state.sprite_batch_data.world_buffer_ptr->uv_offset_scale = sprite_data.uv_offset_scale;
            state.sprite_batch_data.world_buffer_ptr->color = sprite_data.color;
            state.sprite_batch_data.world_buffer_ptr->outline_color = sprite_data.outline_color;
            state.sprite_batch_data.world_buffer_ptr->outline_thickness = sprite_data.outline_thickness;
            state.sprite_batch_data.world_buffer_ptr->flags = flags;
            state.sprite_batch_data.world_buffer_ptr++;

            ++sprite_count;
            ++sprite_total_count;

            sprite_prev_texture = sprite_data.texture;
        }
    }

    if (sprite_count > 0) {
        state.world_flush_queue.push_back(FlushData {
            .texture = sprite_prev_texture,
            .offset = sprite_vertex_offset,
            .count = sprite_count,
            .type = FlushDataType::Sprite
        });
    }

    if (sprite_total_count > 0) {
        const size_t size = (size_t) state.sprite_batch_data.world_buffer_ptr - (size_t) state.sprite_batch_data.world_buffer;
        UpdateBuffer(state.sprite_batch_data.world_instance_buffer, state.sprite_batch_data.world_buffer, size);
    }
}

static void ApplyDrawCommands() {
    ZoneScopedN("Renderer::ApplyDrawCommands");

    auto* const commands = state.command_buffer;

    int prev_flush_data_type = -1;
    int prev_texture_id = -1;

    for (const FlushData& flush_data : state.flush_queue) {
        if (prev_flush_data_type != static_cast<int>(flush_data.type)) {
            switch (flush_data.type) {
            case FlushDataType::Sprite:
                commands->SetVertexBufferArray(*state.sprite_batch_data.buffer_array);
                commands->SetPipelineState(*state.sprite_batch_data.pipeline);
            break;

            case FlushDataType::Glyph:
                commands->SetVertexBufferArray(*state.glyph_batch_data.buffer_array);
                commands->SetPipelineState(*state.glyph_batch_data.pipeline);
            break;

            case FlushDataType::NinePatch:
                commands->SetVertexBufferArray(*state.ninepatch_batch_data.buffer_array);
                commands->SetPipelineState(*state.ninepatch_batch_data.pipeline);
            break;
            }

            commands->SetResource(0, *state.constant_buffer);
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

static void ApplyWorldDrawCommands() {
    ZoneScopedN("Renderer::ApplyWorldDrawCommands");

    auto* const commands = state.command_buffer;

    commands->SetVertexBufferArray(*state.sprite_batch_data.world_buffer_array);
    commands->SetPipelineState(*state.sprite_batch_data.pipeline);
    commands->SetResource(0, *state.constant_buffer);

    int prev_texture_id = -1;

    for (const FlushData& flush_data : state.world_flush_queue) {
        if (flush_data.type != FlushDataType::Sprite) continue;

        if (prev_texture_id != flush_data.texture.id()) {
            commands->SetResource(1, flush_data.texture);
            commands->SetResource(2, Assets::GetSampler(flush_data.texture));
        }
        
        commands->DrawInstanced(4, 0, flush_data.count, flush_data.offset);

        prev_texture_id = flush_data.texture.id();
    }
}

void Renderer::Render(const Camera& camera, const World& world, bool window_resized) {
    ZoneScopedN("Renderer::Render");

    auto* const commands = state.command_buffer;
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
        .max_depth = static_cast<float>(state.max_main_depth),
        .max_world_depth = static_cast<float>(state.max_world_depth),
    };

    commands->UpdateBuffer(*state.constant_buffer, 0, &projections_uniform, sizeof(projections_uniform));

    SortDrawCommands();
    SortWorldDrawCommands();

    state.particle_renderer.compute();
    state.particle_renderer.prepare();

    LLGL::ClearValue clear_value = LLGL::ClearValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f);

    state.compute_light_timer.tick(Time::delta());

    if (state.compute_light_timer.finished()) {
        commands->BeginRenderPass(*state.world_renderer.light_texture_target());
            commands->Clear(LLGL::ClearFlags::Color, clear_value);
        commands->EndRenderPass();

        state.world_renderer.compute_light(camera, world);
    }

    if (state.compute_light_timer.finished() || world.is_lightmap_changed() || window_resized) {
        commands->BeginRenderPass(*state.static_lightmap_target);
            commands->Clear(LLGL::ClearFlags::Color, clear_value);
            state.world_renderer.render_lightmap(camera);
        commands->EndRenderPass();
    }

    commands->BeginRenderPass(*state.background_render_target);
        commands->Clear(LLGL::ClearFlags::ColorDepth, clear_value);
        state.background_renderer.render();
    commands->EndRenderPass();

    commands->BeginRenderPass(*state.world_render_target);
        commands->Clear(LLGL::ClearFlags::ColorDepth, clear_value);
        state.background_renderer.render_world();
        state.world_renderer.render(world.chunk_manager());
        state.particle_renderer.render_world();

        ApplyWorldDrawCommands();

    commands->EndRenderPass();

    commands->BeginRenderPass(*state.swap_chain);
        commands->Clear(LLGL::ClearFlags::Color, LLGL::ClearValue(1.0f, 0.0f, 0.0f, 1.0f, 0.0f));

        commands->SetVertexBuffer(*state.fullscreen_triangle_vertex_buffer);
        commands->SetPipelineState(*state.postprocess_pipeline);
        commands->SetResourceHeap(*state.resource_heap);

        commands->Draw(3, 0);

        state.particle_renderer.render();

        ApplyDrawCommands();
    commands->EndRenderPass();

    commands->End();
    queue->Submit(*commands);

    state.swap_chain->Present();

    state.particle_renderer.reset();
}

#if DEBUG
void Renderer::PrintDebugInfo() {
    LLGL::FrameProfile profile;
    state.debugger->FlushProfile(&profile);

    LOG_DEBUG("Draw commands count: %u", profile.commandBufferRecord.drawCommands);
}
#endif

void Renderer::BeginDepth(Depth depth) {
    state.depth = depth.value;
    state.depth_mode = true;
}

void Renderer::EndDepth() {
    state.depth = -1;
    state.depth_mode = false;
}

inline static void add_sprite_to_batch(const Sprite& sprite, RenderLayer layer, bool is_ui, Depth depth) {
    glm::vec4 uv_offset_scale = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);

    if (sprite.flip_x()) {
        uv_offset_scale.x += uv_offset_scale.z;
        uv_offset_scale.z *= -1.0f;
    }

    if (sprite.flip_y()) {
        uv_offset_scale.y += uv_offset_scale.w;
        uv_offset_scale.w *= -1.0f;
    }

    if (layer == RenderLayer::World) {
        draw_world_sprite(sprite, uv_offset_scale, sprite.texture(), depth);
    } else {
        draw_sprite(sprite, uv_offset_scale, sprite.texture(), is_ui, depth);
    }
}

inline static void add_atlas_sprite_to_batch(const TextureAtlasSprite& sprite, RenderLayer layer, bool is_ui, Depth depth) {
    const math::Rect& rect = sprite.atlas().get_rect(sprite.index());

    glm::vec4 uv_offset_scale = glm::vec4(
        rect.min.x / sprite.atlas().texture().width(),
        rect.min.y / sprite.atlas().texture().height(),
        rect.size().x / sprite.atlas().texture().width(),
        rect.size().y / sprite.atlas().texture().height()
    );

    if (sprite.flip_x()) {
        uv_offset_scale.x += uv_offset_scale.z;
        uv_offset_scale.z *= -1.0f;
    }

    if (sprite.flip_y()) {
        uv_offset_scale.y += uv_offset_scale.w;
        uv_offset_scale.w *= -1.0f;
    }

    if (layer == RenderLayer::World) {
        draw_world_sprite(sprite, uv_offset_scale, sprite.atlas().texture(), depth);
    } else {
        draw_sprite(sprite, uv_offset_scale, sprite.atlas().texture(), is_ui, depth);
    }
}

inline static void add_ninepatch_to_batch(const NinePatch& ninepatch, RenderLayer, bool is_ui, Depth depth) {
    glm::vec4 uv_offset_scale = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);

    if (ninepatch.flip_x()) {
        uv_offset_scale.x += uv_offset_scale.z;
        uv_offset_scale.z *= -1.0f;
    }

    if (ninepatch.flip_y()) {
        uv_offset_scale.y += uv_offset_scale.w;
        uv_offset_scale.w *= -1.0f;
    }
    
    draw_ninepatch(ninepatch, uv_offset_scale, ninepatch.texture(), is_ui, depth);
}

void Renderer::DrawSprite(const Sprite& sprite, RenderLayer render_layer, Depth depth) {
    ZoneScopedN("Renderer::DrawSprite");

    const math::Rect aabb = sprite.calculate_aabb();

    if (sprite.ignore_camera_zoom() && !state.nozoom_camera_frustum.intersects(aabb)) return;
    if (!sprite.ignore_camera_zoom() && !state.camera_frustum.intersects(aabb)) return;

    add_sprite_to_batch(sprite, render_layer, false, depth);
}

void Renderer::DrawSpriteUI(const Sprite& sprite, Depth depth) {
    ZoneScopedN("Renderer::DrawSpriteUI");

    const math::Rect aabb = sprite.calculate_aabb();
    if (!state.ui_frustum.intersects(aabb)) return;

    add_sprite_to_batch(sprite, RenderLayer::Main, true, depth);
}

void Renderer::DrawAtlasSprite(const TextureAtlasSprite& sprite, RenderLayer render_layer, Depth depth) {
    ZoneScopedN("Renderer::DrawAtlasSprite");

    const math::Rect aabb = sprite.calculate_aabb();

    if (sprite.ignore_camera_zoom() && !state.nozoom_camera_frustum.intersects(aabb)) return;
    if (!sprite.ignore_camera_zoom() && !state.camera_frustum.intersects(aabb)) return;

    add_atlas_sprite_to_batch(sprite, render_layer, false, depth);
}

void Renderer::DrawAtlasSpriteUI(const TextureAtlasSprite& sprite, Depth depth) {
    ZoneScopedN("Renderer::DrawAtlasSpriteUI");

    const math::Rect aabb = sprite.calculate_aabb();
    if (!state.ui_frustum.intersects(aabb)) return;

    add_atlas_sprite_to_batch(sprite, RenderLayer::Main, true, depth);
}

void Renderer::DrawNinePatchUI(const NinePatch& ninepatch, Depth depth) {
    ZoneScopedN("Renderer::DrawNinePatchUI");

    const math::Rect aabb = ninepatch.calculate_aabb();
    if (!state.ui_frustum.intersects(aabb)) return;

    add_ninepatch_to_batch(ninepatch, RenderLayer::Main, true, depth);
}

void Renderer::DrawText(const RichTextSection* sections, size_t size, const glm::vec2& position, FontAsset key, bool is_ui, Depth depth) {
    ZoneScopedN("Renderer::DrawText");

    const Font& font = Assets::GetFont(key);

    uint32_t& depth_index = state.main_depth_index;

    if (state.depth_mode) depth = state.depth;

    const int order = depth.value < 0 ? depth_index : depth.value;
    const uint32_t new_index = depth.value < 0 ? depth_index + 1 : depth.value;
    if (depth.advance) depth_index = std::max(depth_index, static_cast<uint32_t>(new_index));
    if (static_cast<uint32_t>(order) > state.max_main_depth) state.max_main_depth = order;

    float x = position.x;
    float y = position.y;

    for (size_t i = 0; i < size; ++i) {
        const RichTextSection section = sections[i];
        const char* str = section.text.data();
        const size_t length = section.text.size();
        const float scale = section.size / font.font_size;

        for (size_t i = 0; i < length;) {
            const uint32_t c = next_utf8_codepoint(str, i);

            if (c == '\n') {
                y += section.size;
                x = position.x;
                continue;
            }

            const Glyph& ch = font.glyphs.find(c)->second;

            if (c == ' ') {
                x += (ch.advance >> 6) * scale;
                continue;
            }

            const float xpos = x + ch.bearing.x * scale; // ⌄ Make the origin at the top left corner
            const float ypos = y - ch.bearing.y * scale + section.size - font.ascender * scale;
            const glm::vec2 pos = glm::vec2(xpos, ypos);
            const glm::vec2 size = glm::vec2(ch.size) * scale;

            draw_glyph(pos, size, section.color, font.texture, ch.texture_coords, ch.tex_size, is_ui, order);

            x += (ch.advance >> 6) * scale;
        }
    }
}

void Renderer::DrawBackground(const BackgroundLayer& layer) {
    ZoneScopedN("Renderer::DrawBackground");

    const math::Rect aabb = math::Rect::from_top_left(layer.position() - layer.anchor().to_vec2() * layer.size(), layer.size());

    if (layer.nonscale() && !state.nozoom_camera_frustum.intersects(aabb)) return;
    if (!layer.nonscale() && !state.camera_frustum.intersects(aabb)) return;

    if (layer.is_world()) {
        state.background_renderer.draw_world_layer(layer);
    } else {
        state.background_renderer.draw_layer(layer);
    }
}

void Renderer::DrawParticle(const glm::vec2& position, const glm::quat& rotation, float scale, Particle::Type type, uint8_t variant, Depth depth, bool world) {
    ZoneScopedN("Renderer::DrawParticle");

    if (world)
        state.particle_renderer.draw_particle_world(position, rotation, scale, type, variant, depth);
    else
        state.particle_renderer.draw_particle(position, rotation, scale, type, variant, depth);
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
    image_view.dataType = data_type;
    image_view.data = data;
    image_view.dataSize = width * height * layers * components;

    uint32_t id = state.texture_index++;

    return Texture(id, sampler, glm::uvec2(width, height), state.context->CreateTexture(texture_desc, &image_view));
}

LLGL::Shader* Renderer::LoadShader(const ShaderPath& shader_path, const std::vector<ShaderDef>& shader_defs, const std::vector<LLGL::VertexAttribute>& vertex_attributes) {
    ZoneScopedN("Renderer::LoadShader");

    const RenderBackend backend = state.backend;
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

    LLGL::Shader* shader = state.context->CreateShader(shader_desc);
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

    RESOURCE_RELEASE(state.sprite_batch_data.vertex_buffer)
    RESOURCE_RELEASE(state.sprite_batch_data.instance_buffer)
    RESOURCE_RELEASE(state.sprite_batch_data.buffer_array)
    RESOURCE_RELEASE(state.sprite_batch_data.pipeline)
    RESOURCE_RELEASE(state.sprite_batch_data.world_instance_buffer)
    RESOURCE_RELEASE(state.sprite_batch_data.world_buffer_array)
    delete[] state.sprite_batch_data.buffer;
    delete[] state.sprite_batch_data.world_buffer;

    RESOURCE_RELEASE(state.glyph_batch_data.vertex_buffer)
    RESOURCE_RELEASE(state.glyph_batch_data.instance_buffer)
    RESOURCE_RELEASE(state.glyph_batch_data.buffer_array)
    RESOURCE_RELEASE(state.glyph_batch_data.pipeline)
    delete[] state.glyph_batch_data.buffer;

    RESOURCE_RELEASE(state.ninepatch_batch_data.vertex_buffer)
    RESOURCE_RELEASE(state.ninepatch_batch_data.instance_buffer)
    RESOURCE_RELEASE(state.ninepatch_batch_data.buffer_array)
    RESOURCE_RELEASE(state.ninepatch_batch_data.pipeline)
    delete[] state.ninepatch_batch_data.buffer;

    state.world_renderer.terminate();
    state.background_renderer.terminate();
    state.particle_renderer.terminate();

    RESOURCE_RELEASE(state.constant_buffer);
    RESOURCE_RELEASE(state.command_buffer);
    RESOURCE_RELEASE(state.swap_chain);

    Assets::DestroyTextures();
    Assets::DestroySamplers();

    LLGL::RenderSystem::Unload(std::move(state.context));
}
