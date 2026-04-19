#include "renderer.hpp"

#include <cstdint>

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

#include <SGE/engine.hpp>
#include <SGE/renderer/macros.hpp>
#include <SGE/renderer/batch.hpp>
#include <SGE/types/binding_layout.hpp>
#include <SGE/types/blend_mode.hpp>
#include <SGE/profile.hpp>

#include "../assets.hpp"

#include "world_renderer.hpp"
#include "background_renderer.hpp"
#include "particle_renderer.hpp"

static constexpr int CAMERA_FRUSTUM = 0;
static constexpr int NOZOOM_CAMERA_FRUSTUM = 1;

struct SGE_ALIGN(16) PostProcessUniforms {
    glm::vec2 uv_scale;
    glm::vec2 uv_offset;
};

uint32_t GameRenderer::GetMainOrderIndex() { return m_main_batch->Order(); }
uint32_t GameRenderer::GetWorldOrderIndex() { return m_world_batch->Order(); }
const sge::Unique<LLGL::Buffer>& GameRenderer::ChunkVertexBuffer() { return m_chunk_vertex_buffer; }

GameRenderer::GameRenderer(const std::shared_ptr<sge::Renderer>& renderer) :
    m_particle_renderer(renderer),
    m_world_renderer(renderer),
    m_background_renderer(renderer),
    m_renderer(renderer)
{
    const auto& render_context = renderer->GetRenderContext();

    const sge::Vertex vertices[] = {
        sge::Vertex(0.0, 0.0),
        sge::Vertex(0.0, 1.0),
        sge::Vertex(1.0, 0.0),
        sge::Vertex(1.0, 1.0),
    };

    m_chunk_vertex_buffer = render_context->CreateVertexBuffer(vertices, Assets::GetVertexFormat(VertexFormatAsset::TilemapVertex), "WorldRenderer VertexBuffer");

    LLGL::PipelineLayoutDescriptor pipelineLayoutDesc;
    pipelineLayoutDesc.staticSamplers = {
        LLGL::StaticSamplerDescriptor("BackgroundTextureSampler", LLGL::StageFlags::FragmentStage, 5, Assets::GetSampler(sge::TextureSampler::Nearest)->descriptor()),
        LLGL::StaticSamplerDescriptor("WorldTextureSampler", LLGL::StageFlags::FragmentStage, 7, Assets::GetSampler(sge::TextureSampler::Nearest)->descriptor()),
        LLGL::StaticSamplerDescriptor("LightMapSampler", LLGL::StageFlags::FragmentStage, 9, Assets::GetSampler(sge::TextureSampler::Nearest)->descriptor()),
        LLGL::StaticSamplerDescriptor("LightSampler", LLGL::StageFlags::FragmentStage, 11, Assets::GetSampler(sge::TextureSampler::Nearest)->descriptor()),
    };
    pipelineLayoutDesc.combinedTextureSamplers = {
        LLGL::CombinedTextureSamplerDescriptor{ "BackgroundTexture", "BackgroundTexture", "BackgroundTextureSampler", 4 },
        LLGL::CombinedTextureSamplerDescriptor{ "WorldTexture", "WorldTexture", "WorldTextureSampler", 6 },
        LLGL::CombinedTextureSamplerDescriptor{ "LightMap", "LightMap", "LightMapSampler", 8 },
        LLGL::CombinedTextureSamplerDescriptor{ "Light", "Light", "LightSampler", 10 },
    };
    pipelineLayoutDesc.heapBindings = sge::BindingLayout({
        sge::BindingLayoutItem::ConstantBuffer(2, "GlobalUniformBuffer", LLGL::StageFlags::VertexStage),
        sge::BindingLayoutItem::ConstantBuffer(3, "UniformBuffer", LLGL::StageFlags::VertexStage),
        sge::BindingLayoutItem::Texture(4, "BackgroundTexture", LLGL::StageFlags::FragmentStage),
        sge::BindingLayoutItem::Texture(6, "WorldTexture", LLGL::StageFlags::FragmentStage),
        sge::BindingLayoutItem::Texture(8, "LightMap", LLGL::StageFlags::FragmentStage),
        sge::BindingLayoutItem::Texture(10, "Light", LLGL::StageFlags::FragmentStage),
    });

    sge::Ref<LLGL::PipelineLayout> pipelineLayout = render_context->CreatePipelineLayout(pipelineLayoutDesc);

    {
        m_postprocess_uniform_buffer = render_context->CreateConstantBuffer(sizeof(PostProcessUniforms));
        const glm::vec2 vertices[] = {
            glm::vec2(-1.0f, 1.0f),  glm::vec2(0.0f, 0.0f),
            glm::vec2(3.0f,  1.0f),  glm::vec2(2.0f, 0.0f),
            glm::vec2(-1.0f, -3.0f), glm::vec2(0.0f, 2.0f),
        };
        m_postprocess_vertex_buffer = render_context->CreateVertexBuffer(vertices, Assets::GetVertexFormat(VertexFormatAsset::PostProcessVertex));
    }

    m_resource_heap = render_context->CreateResourceHeap(pipelineLayout, {
        m_renderer->GlobalUniformBuffer().Get(),
        m_postprocess_uniform_buffer.Get(),
        m_background_renderer.target_texture().Get(),
        m_world_renderer.target_texture().Get(),
        m_world_renderer.static_lightmap_texture().Get(),
        m_world_renderer.light_texture().Get()
    });

    const sge::ShaderPipeline& postprocess_shader = Assets::GetShader(ShaderAsset::PostProcessShader);

    sge::GraphicsPipelineConfig pipelineConfig;
    pipelineConfig.debugName = "LightMap Pipeline";
    pipelineConfig.vertexShader = postprocess_shader.vs;
    pipelineConfig.pixelShader = postprocess_shader.ps;
    pipelineConfig.layout = pipelineLayout;
    pipelineConfig.indexFormat = LLGL::Format::R16UInt;
    pipelineConfig.primitiveTopology = LLGL::PrimitiveTopology::TriangleStrip;

    m_postprocess_pipeline = render_context->CreatePipelineState(pipelineConfig);

    m_background_renderer.init_world(m_world_renderer);

    const sge::ShaderPipeline& font_shader = Assets::GetShader(ShaderAsset::FontShader);
    m_main_batch = m_renderer->CreateBatch({
        .font_shader = font_shader.ps
    });
    m_world_batch = m_renderer->CreateBatch({
        .font_shader = font_shader.ps
    });
    m_ui_batch = m_renderer->CreateBatch({
        .font_shader = font_shader.ps,
        .enable_scissor = true
    });

    m_world_batch->SetDepthEnabled(true);
    m_ui_batch->SetIsUi(true);
}

void GameRenderer::ResizeTextures(LLGL::Extent2D size) {
    const auto& context = m_renderer->GetRenderContext()->GetLLGLContext();

    m_world_renderer.init_targets(size);
    m_background_renderer.init_targets(size);
    m_world_renderer.init_textures(size);

    if (m_resource_heap.IsValid()) {
        context->WriteResourceHeap(*m_resource_heap, 2, {
            m_background_renderer.target_texture().Get(),
            m_world_renderer.target_texture().Get(),
            m_world_renderer.static_lightmap_texture().Get(),
            m_world_renderer.light_texture().Get()
        });
    }
}

void GameRenderer::InitWorldRenderer(const WorldData &world) {
    // m_world_renderer.init_lightmap_chunks(world);
    m_world_renderer.init_lighting(world);
}

void GameRenderer::UpdateLight() {
    m_update_light = true;
}

void GameRenderer::Begin(const sge::Camera& camera, World& world) {
    ZoneScoped;

    const auto& render_context = m_renderer->GetRenderContext();

    const auto& commands = m_renderer->CommandBuffer();
    auto* const command_queue = m_renderer->CommandQueue();

    const sge::Rect camera_frustum = sge::Rect::from_corners(
        camera.position() + camera.get_projection_area().min,
        camera.position() + camera.get_projection_area().max
    );
    const sge::Rect nozoom_camera_frustum = sge::Rect::from_corners(
        camera.position() + camera.get_nozoom_projection_area().min,
        camera.position() + camera.get_nozoom_projection_area().max
    );
    const sge::Rect ui_frustum = sge::Rect::from_corners(glm::vec2(0.0), camera.viewport());

    m_camera_frustums[CAMERA_FRUSTUM] = camera_frustum;
    m_camera_frustums[NOZOOM_CAMERA_FRUSTUM] = nozoom_camera_frustum;
    m_ui_frustum = ui_frustum;

    m_world_renderer.update(world);

    if (m_update_light) {
        commands->Begin();
            commands->BeginRenderPass(render_context->GetOrCreateRenderTarget(m_world_renderer.light_texture_target(), camera.samples()));
                m_renderer->Clear(LLGL::ClearValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f), LLGL::ClearFlags::Color);
            commands->EndRenderPass();
        commands->End();
        command_queue->Submit(*commands);
    }

    m_renderer->Begin();

    {
        const glm::vec2 current_size = camera.get_projection_area().size();
        const glm::vec2 max_size = glm::vec2(camera.viewport()) * Constants::CAMERA_MIN_ZOOM;
        const glm::vec2 a = (current_size / Constants::TILE_SIZE);
        const glm::vec2 b = (max_size / Constants::TILE_SIZE);

        PostProcessUniforms uniforms = {
            .uv_scale = a / b,
            .uv_offset = (max_size - current_size) * 0.5f / max_size,
        };
        commands->UpdateBuffer(*m_postprocess_uniform_buffer, 0, &uniforms, sizeof(uniforms));
    }

    if (m_update_light) {
        m_world_renderer.compute_light(camera, world);
    }

    m_main_batch->Reset();
    m_world_batch->Reset();
    m_ui_batch->Reset();
}

void GameRenderer::Render(const std::shared_ptr<sge::GlfwWindow>& window, const sge::Camera& camera, const World& world) {
    ZoneScoped;

    const auto& render_context = m_renderer->GetRenderContext();
    const auto& commands = m_renderer->CommandBuffer();

    m_particle_renderer.compute();
    m_particle_renderer.prepare();

    LLGL::ClearValue clear_value = LLGL::ClearValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f);

    m_renderer->PrepareBatch(*m_main_batch);
    m_renderer->PrepareBatch(*m_world_batch);
    m_renderer->PrepareBatch(*m_ui_batch);

    m_renderer->UploadBatchData();

    if (m_update_light) {
        m_renderer->BeginPass(render_context->GetOrCreateRenderTarget(m_world_renderer.static_lightmap_target(), camera.samples()), camera);
            m_renderer->Clear(clear_value, LLGL::ClearFlags::Color);
            m_world_renderer.render_lightmap(world.chunk_manager());
        m_renderer->EndPass();

        m_update_light = false;
    }

    // m_renderer->BeginPass(*m_background_renderer.target());
    //     m_renderer->Clear(clear_value, LLGL::ClearFlags::Color);
    //     m_background_renderer.render();
    // m_renderer->EndPass();

    m_renderer->BeginPass(render_context->GetOrCreateRenderTarget(m_world_renderer.target(), camera.samples()), camera);
        m_renderer->Clear(clear_value, LLGL::ClearFlags::ColorDepth);

        m_background_renderer.render();

        m_background_renderer.render_world();
        m_world_renderer.render(world.chunk_manager());
        m_particle_renderer.render_world();

        m_renderer->RenderBatch(*m_world_batch);
    m_renderer->EndPass();

    m_renderer->BeginPass(window, camera);
        m_renderer->Clear(clear_value);

        commands->SetVertexBuffer(*m_postprocess_vertex_buffer);
        commands->SetPipelineState(render_context->GetOrCreatePipeline(m_postprocess_pipeline));
        commands->SetResourceHeap(*m_resource_heap);
        commands->Draw(3, 0);

        m_particle_renderer.render();

        m_renderer->RenderBatch(*m_main_batch);
        m_renderer->RenderBatch(*m_ui_batch);
    m_renderer->EndPass();

    m_renderer->End();

    m_renderer->Present(window);

    m_particle_renderer.reset();
    m_background_renderer.reset();
}

void GameRenderer::BeginOrderMode(int order, bool advance) noexcept {
    m_main_batch->BeginOrderMode(order, advance);
    m_world_batch->BeginOrderMode(order, advance);
    m_ui_batch->BeginOrderMode(order, advance);
}

void GameRenderer::EndOrderMode() noexcept {
    m_main_batch->EndOrderMode();
    m_world_batch->EndOrderMode();
    m_ui_batch->EndOrderMode();
}

void GameRenderer::BeginBlendMode(sge::BlendMode blend_mode) noexcept {
    m_main_batch->BeginBlendMode(blend_mode);
    m_world_batch->BeginBlendMode(blend_mode);
    m_ui_batch->BeginBlendMode(blend_mode);
}

void GameRenderer::EndBlendMode() noexcept {
    m_main_batch->EndBlendMode();
    m_world_batch->EndBlendMode();
    m_ui_batch->EndBlendMode();
}

uint32_t GameRenderer::DrawSprite(const sge::Sprite& sprite, sge::Order order) {
    ZoneScoped;

    const sge::Rect aabb = sprite.calculate_aabb();
    if (!m_camera_frustums[sprite.ignore_camera_zoom()].intersects(aabb)) return 0;

    return m_main_batch->DrawSprite(sprite, order);
}

uint32_t GameRenderer::DrawSpriteWorld(const sge::Sprite& sprite, sge::Order order) {
    ZoneScoped;

    const sge::Rect aabb = sprite.calculate_aabb();
    if (!m_camera_frustums[sprite.ignore_camera_zoom()].intersects(aabb)) return 0;

    return m_world_batch->DrawSprite(sprite, order);
}

uint32_t GameRenderer::DrawSpriteUI(const sge::Sprite& sprite, sge::Order order) {
    ZoneScoped;

    const sge::Rect aabb = sprite.calculate_aabb();
    if (!m_ui_frustum.intersects(aabb)) return 0;

    return m_ui_batch->DrawSprite(sprite, order);
}

uint32_t GameRenderer::DrawAtlasSprite(const sge::TextureAtlasSprite& sprite, sge::Order order) {
    ZoneScoped;

    const sge::Rect aabb = sprite.calculate_aabb();
    if (!m_camera_frustums[sprite.ignore_camera_zoom()].intersects(aabb)) return 0;

    return m_main_batch->DrawAtlasSprite(sprite, order);
}

uint32_t GameRenderer::DrawAtlasSpriteWorld(const sge::TextureAtlasSprite& sprite, sge::Order order) {
    ZoneScoped;

    const sge::Rect aabb = sprite.calculate_aabb();
    if (!m_camera_frustums[sprite.ignore_camera_zoom()].intersects(aabb)) return 0;

    return m_world_batch->DrawAtlasSprite(sprite, order);
}

uint32_t GameRenderer::DrawAtlasSpriteWorldPremultiplied(const sge::TextureAtlasSprite& sprite, sge::Order order) {
    ZoneScoped;

    const sge::Rect aabb = sprite.calculate_aabb();
    if (!m_camera_frustums[sprite.ignore_camera_zoom()].intersects(aabb)) return 0;

    m_world_batch->BeginBlendMode(sge::BlendMode::PremultipliedAlpha);
    uint32_t ordr = m_world_batch->DrawAtlasSprite(sprite, order);
    m_world_batch->EndBlendMode();

    return ordr;
}

uint32_t GameRenderer::DrawAtlasSpriteUI(const sge::TextureAtlasSprite& sprite, sge::Order order) {
    ZoneScoped;

    const sge::Rect aabb = sprite.calculate_aabb();
    if (!m_ui_frustum.intersects(aabb)) return 0;

    return m_ui_batch->DrawAtlasSprite(sprite, order);
}

uint32_t GameRenderer::DrawNinePatchUI(const sge::NinePatch& ninepatch, sge::Order order) {
    ZoneScoped;

    const sge::Rect aabb = ninepatch.calculate_aabb();
    if (!m_ui_frustum.intersects(aabb)) return 0;

    return m_ui_batch->DrawNinePatch(ninepatch, order);
}

uint32_t GameRenderer::DrawText(const sge::RichTextSection* sections, size_t size, const glm::vec2& position, const sge::Font& font, sge::Order order) {
    ZoneScoped;

    return m_main_batch->DrawText(sections, size, position, font, order);
}

uint32_t GameRenderer::DrawTextUI(const sge::RichTextSection* sections, size_t size, const glm::vec2& position, const sge::Font& font, sge::Order order) {
    ZoneScoped;

    return m_ui_batch->DrawText(sections, size, position, font, order);
}

void GameRenderer::DrawBackground(const BackgroundLayer& layer) {
    ZoneScoped;

    const sge::Rect aabb = sge::Rect::from_top_left(layer.position() - layer.anchor().to_vec2() * layer.size(), layer.size());
    if (!m_camera_frustums[layer.nonscale()].intersects(aabb)) return;

    if (layer.is_world()) {
        m_background_renderer.draw_world_layer(layer);
    } else {
        m_background_renderer.draw_layer(layer);
    }
}

void GameRenderer::DrawParticle(const glm::vec2& position, const glm::quat& rotation, float scale, Particle::Type type, uint8_t variant, sge::Order order, bool world) {
    ZoneScoped;

    if (world)
        m_particle_renderer.draw_particle_world(position, rotation, scale, type, variant, order);
    else
        m_particle_renderer.draw_particle(position, rotation, scale, type, variant, order);
}

GameRenderer::~GameRenderer() {
    if (m_main_batch)
        m_renderer->DestroyBatch(*m_main_batch);

    if (m_world_batch)
        m_renderer->DestroyBatch(*m_world_batch);
    
    if (m_ui_batch)
        m_renderer->DestroyBatch(*m_ui_batch);

    const auto& render_context = m_renderer->GetRenderContext();
    render_context->DeletePipeline(m_postprocess_pipeline);
}