#include "background.hpp"

#include <vector>
#include <LLGL/LLGL.h>

#include "LLGL/Format.h"
#include "LLGL/PipelineState.h"
#include "LLGL/ResourceFlags.h"
#include "assets.hpp"
#include "constants.hpp"
#include "log.hpp"
#include "renderer/assets.hpp"
#include "renderer/renderer.hpp"
#include "renderer/utils.hpp"
#include "types/anchor.hpp"
#include "types/texture.hpp"

using Constants::TILE_SIZE;

constexpr uint32_t MAX_QUADS = 500;
constexpr uint32_t MAX_INDICES = MAX_QUADS * 6;
constexpr uint32_t MAX_VERTICES = MAX_QUADS * 4;

class Layer {
public:
    Layer(TextureKey key, float scale) {
        const Texture& texture = Assets::GetTexture(key);

        m_texture = texture;
        m_scale = scale;
        m_size = glm::vec2(texture.size) * m_scale;
    }

    inline Layer& set_width(float width) { m_size.x = width; return *this; }
    inline Layer& set_height(float height) { m_size.y = height; return *this; }
    inline Layer& set_position(const glm::vec2& position) { m_position = position; return *this; }
    inline Layer& set_speed(float x, float y) { m_speed = glm::vec2(x, y); return *this; }
    inline Layer& set_x(float x) { m_x = x; return *this; }
    inline Layer& set_y(float y) { m_y = y;  return *this; }
    inline Layer& set_anchor(const Anchor anchor) { m_anchor = anchor; return *this; }
    inline Layer& set_nonscale(bool nonscale) { m_nonscale = nonscale; return *this; }
    inline Layer& set_follow_camera(bool follow) { m_follow_camera = follow; return *this; }
    inline Layer& set_fill_screen_height() { m_fill_screen_height = true; return *this; }
    inline Layer& set_fill_screen_width() { m_fill_screen_width = true; return *this; }

    [[nodiscard]] inline const Texture& texture() const { return m_texture; }
    [[nodiscard]] inline const glm::vec2& position() const { return m_position; }
    [[nodiscard]] inline const glm::vec2& speed() const { return m_speed; }
    [[nodiscard]] inline const glm::vec2& size() const { return m_size; }
    [[nodiscard]] inline float scale() const { return m_scale; }
    [[nodiscard]] inline float x() const { return m_x; }
    [[nodiscard]] inline float y() const { return m_y; }
    [[nodiscard]] inline Anchor anchor() const { return m_anchor; }
    [[nodiscard]] inline uint32_t texture_id() const { return m_texture.id; }
    [[nodiscard]] inline bool nonscale() const { return m_nonscale; }
    [[nodiscard]] inline bool follow_camera() const { return m_follow_camera; }
    [[nodiscard]] inline bool fill_screen_height() const { return m_fill_screen_height; }
    [[nodiscard]] inline bool fill_screen_width() const { return m_fill_screen_width; }

private:
    Texture m_texture;
    glm::vec2 m_position = glm::vec2(0.0f);
    glm::vec2 m_speed = glm::vec2(0.0f);
    glm::vec2 m_size = glm::vec2(0.0f);
    float m_scale = 0.0f;
    float m_x;
    float m_y;
    Anchor m_anchor;
    bool m_nonscale = true;
    bool m_follow_camera = true;
    bool m_fill_screen_height = false;
    bool m_fill_screen_width = false;
};

struct BackgroundVertex {
    glm::vec2 position;
    glm::vec2 uv;
    glm::vec2 size;
    glm::vec2 tex_size;
    glm::vec2 speed;
    int nonscale;
};

struct BackgroundUniform {
    glm::vec2 camera_position;
    glm::vec2 window_size;
};

static struct BackgroundState {
    std::vector<Layer> layers;
    LLGL::PipelineState* pipeline = nullptr;
    LLGL::Buffer* vertex_buffer = nullptr;
    LLGL::Buffer* index_buffer = nullptr;
    LLGL::Buffer* uniform_buffer = nullptr;
    BackgroundVertex* buffer = nullptr;
    BackgroundVertex* buffer_ptr = nullptr;
} state;

void setup_cavern_background(const World& world);
void setup_sky_background();

void Background::InitRenderer() {
    const auto& context = Renderer::Context();
    const auto* swap_chain = Renderer::SwapChain();

    state.buffer = new BackgroundVertex[MAX_VERTICES];
    state.buffer_ptr = state.buffer;

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

    state.vertex_buffer = CreateVertexBuffer(MAX_VERTICES * sizeof(BackgroundVertex), BackgroundVertexFormat(), "BackgroundBatch VertexBuffer");
    state.index_buffer = CreateIndexBuffer(indices, LLGL::Format::R32UInt, "BackgroundBatch IndexBuffer");
    state.uniform_buffer = CreateConstantBuffer(sizeof(BackgroundUniform));

    const uint32_t samplerBinding = Renderer::Backend().IsOpenGL() ? 3 : 4;

    LLGL::PipelineLayoutDescriptor pipelineLayoutDesc;
    pipelineLayoutDesc.bindings = {
        LLGL::BindingDescriptor(
            "UniformBuffer",
            LLGL::ResourceType::Buffer,
            LLGL::BindFlags::ConstantBuffer,
            LLGL::StageFlags::FragmentStage,
            LLGL::BindingSlot(1)
        ),
        LLGL::BindingDescriptor(
            "BackgroundUniformBuffer",
            LLGL::ResourceType::Buffer,
            LLGL::BindFlags::ConstantBuffer,
            LLGL::StageFlags::FragmentStage,
            LLGL::BindingSlot(2)
        ),
        LLGL::BindingDescriptor("u_texture", LLGL::ResourceType::Texture, LLGL::BindFlags::Sampled, LLGL::StageFlags::FragmentStage, LLGL::BindingSlot(3)),
        LLGL::BindingDescriptor("u_sampler", LLGL::ResourceType::Sampler, 0, LLGL::StageFlags::FragmentStage, LLGL::BindingSlot(samplerBinding)),
    };

    LLGL::PipelineLayout* pipelineLayout = context->CreatePipelineLayout(pipelineLayoutDesc);

    const ShaderPipeline& background_shader = Assets::GetShader(ShaderAssetKey::BackgroundShader);

    LLGL::GraphicsPipelineDescriptor pipelineDesc;
    pipelineLayoutDesc.debugName = "Background Pipeline";
    pipelineDesc.vertexShader = background_shader.vs;
    pipelineDesc.fragmentShader = background_shader.ps;
    pipelineDesc.geometryShader = background_shader.gs;
    pipelineDesc.pipelineLayout = pipelineLayout;
    pipelineDesc.indexFormat = LLGL::Format::R32UInt;
    pipelineDesc.primitiveTopology = LLGL::PrimitiveTopology::TriangleList;
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

    state.pipeline = context->CreatePipelineState(pipelineDesc);

    if (const LLGL::Report* report = state.pipeline->GetReport()) {
        if (report->HasErrors()) LOG_ERROR("%s", report->GetText());
    }
}

void Background::SetupMenuBackground() {
    const float pos = 150.0f;

    setup_sky_background();

    state.layers.push_back(
        Layer(TextureKey::Background7, 1.5f)
            .set_y(pos)
            .set_speed(1.0f, 0.9f)
    );
    state.layers.push_back(
        Layer(TextureKey::Background90, 1.5f)
            .set_speed(1.0f, 0.8f)
            .set_y(pos - 200.0f)
    );
    state.layers.push_back(
        Layer(TextureKey::Background91, 1.5f)
            .set_speed(1.0f, 0.7f)
            .set_y(pos - 300.0f)
    );
    state.layers.push_back(
        Layer(TextureKey::Background92, 1.5f)
            .set_speed(1.0f, 0.6f)
            .set_y(0.0f)
    );
    state.layers.push_back(
        Layer(TextureKey::Background112, 1.2f)
            .set_speed(1.0f, 0.7f)
            .set_y(0.0f)
    );
}

void Background::SetupWorldBackground(const World& world) {
    state.layers.clear();

    setup_sky_background();

    state.layers.push_back(
        Layer(TextureKey::Background93, 2.0f)
            .set_speed(0.1f, 0.4f)
            .set_y(world.layers().underground * TILE_SIZE)
            .set_anchor(Anchor::BottomCenter)
            .set_fill_screen_width()
    );

    state.layers.push_back(
        Layer(TextureKey::Background114, 2.0f)
            .set_speed(0.15f, 0.5f)
            .set_y((world.layers().underground + world.layers().dirt_height * 0.5f) * TILE_SIZE)
            .set_anchor(Anchor::BottomCenter)
            .set_fill_screen_width()
    );

    state.layers.push_back(
        Layer(TextureKey::Background55, 2.0f)
            .set_speed(0.2f, 0.6f)
            .set_y((world.layers().underground + world.layers().dirt_height * 0.5f) * TILE_SIZE)
            .set_anchor(Anchor::BottomCenter)
            .set_fill_screen_width()
    );

    setup_cavern_background(world);
}

void Background::Update(const Camera &camera) {
    for (Layer& layer : state.layers) {
        glm::vec2 new_position = layer.position();

        new_position.x = layer.follow_camera() ? camera.position().x : layer.x();
        new_position.y = camera.position().y + (layer.y() - camera.position().y) * layer.speed().y;

        layer.set_position(new_position);

        if (layer.fill_screen_height()) {
            layer.set_height(camera.viewport().y);
        }
        if (layer.fill_screen_width()) {
            layer.set_width(camera.viewport().x);
        }

        const glm::vec2 offset = layer.anchor().to_vec2();
        const glm::vec2 pos = new_position - layer.size() * offset;

        const glm::vec2 tex_size = glm::vec2(layer.texture().size);
        state.buffer_ptr->position = pos;
        state.buffer_ptr->uv = glm::vec2(0.0f);
        state.buffer_ptr->size = layer.size();
        state.buffer_ptr->tex_size = tex_size;
        state.buffer_ptr->speed = layer.speed();
        state.buffer_ptr->nonscale = layer.nonscale();
        state.buffer_ptr++;

        state.buffer_ptr->position = pos + glm::vec2(layer.size().x, 0.0f);
        state.buffer_ptr->uv = glm::vec2(1.0f, 0.0f);
        state.buffer_ptr->size = layer.size();
        state.buffer_ptr->tex_size = tex_size;
        state.buffer_ptr->speed = layer.speed();
        state.buffer_ptr->nonscale = layer.nonscale();
        state.buffer_ptr++;

        state.buffer_ptr->position = pos + glm::vec2(0.0f, layer.size().y);
        state.buffer_ptr->uv = glm::vec2(0.0f, 1.0f);
        state.buffer_ptr->size = layer.size();
        state.buffer_ptr->tex_size = tex_size;
        state.buffer_ptr->speed = layer.speed();
        state.buffer_ptr->nonscale = layer.nonscale();
        state.buffer_ptr++;

        state.buffer_ptr->position = pos + layer.size();
        state.buffer_ptr->uv = glm::vec2(1.0f);
        state.buffer_ptr->size = layer.size();
        state.buffer_ptr->tex_size = tex_size;
        state.buffer_ptr->speed = layer.speed();
        state.buffer_ptr->nonscale = layer.nonscale();
        state.buffer_ptr++;
    }
}

void Background::Render(const Camera& camera) {
    if (state.layers.empty()) return;
    
    auto* const commands = Renderer::CommandBuffer();

    auto uniform = BackgroundUniform {
        .camera_position = camera.position(),
        .window_size = camera.viewport()
    };

    commands->UpdateBuffer(*state.uniform_buffer, 0, &uniform, sizeof(BackgroundUniform));

    const ptrdiff_t size = (uint8_t*) state.buffer_ptr - (uint8_t*) state.buffer;
    commands->UpdateBuffer(*state.vertex_buffer, 0, state.buffer, size);
    
    commands->SetVertexBuffer(*state.vertex_buffer);
    commands->SetIndexBuffer(*state.index_buffer);

    commands->SetPipelineState(*state.pipeline);
    commands->SetResource(0, *Renderer::ProjectionsUniformBuffer());
    commands->SetResource(1, *state.uniform_buffer);

    uint32_t i = 0;
    for (const Layer& layer : state.layers) {
        const Texture& t = layer.texture();

        commands->SetResource(2, *t.texture);
        commands->SetResource(3, Assets::GetSampler(t.sampler));
        commands->DrawIndexed(6, 0, i);

        i += 4;
    }

    state.buffer_ptr = state.buffer;
}

void setup_sky_background() {
    state.layers.push_back(
        Layer(TextureKey::Background0, 1.0f)
            .set_anchor(Anchor::Center)
            .set_speed(0.0f, 0.0f)
            .set_y(0.0f)
            .set_fill_screen_height()
            .set_fill_screen_width()
            .set_nonscale(true)
            .set_follow_camera(true)
    );
}

void setup_cavern_background(const World& world) {
    const float underground_level = static_cast<float>(world.layers().underground) * TILE_SIZE + TILE_SIZE;
    const float world_height = static_cast<float>(world.area().height() - (world.area().height() - world.playable_area().height()) / 2) * TILE_SIZE;
    const float world_width = static_cast<float>(world.playable_area().width()) * TILE_SIZE;

    const float speed_x = 0.5f;

    state.layers.push_back(
        Layer(TextureKey::Background77, 1.0f)
            .set_speed(speed_x, 1.0f)
            .set_anchor(Anchor::BottomLeft)
            .set_x(world.playable_area().left() * TILE_SIZE)
            .set_y(underground_level)
            .set_width(world_width)
            .set_nonscale(false)
            .set_follow_camera(false)
    );

    state.layers.push_back(
        Layer(TextureKey::Background78, 1.0f)
            .set_speed(speed_x, 1.0f) 
            .set_anchor(Anchor::TopLeft)
            .set_x(world.playable_area().left() * TILE_SIZE)
            .set_y(underground_level)
            .set_width(world_width)
            .set_height(world_height - underground_level)
            .set_nonscale(false)
            .set_follow_camera(false)
    );
}