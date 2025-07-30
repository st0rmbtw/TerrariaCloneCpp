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
#include "../utils.hpp"

#include "world_renderer.hpp"
#include "background_renderer.hpp"
#include "particle_renderer.hpp"

static constexpr int CAMERA_FRUSTUM = 0;
static constexpr int NOZOOM_CAMERA_FRUSTUM = 1;

static struct RendererState {
    sge::Batch main_batch;
    sge::Batch world_batch;
    sge::Batch ui_batch;

    sge::Rect camera_frustums[2];
    sge::Rect ui_frustum;

    ParticleRenderer particle_renderer;
    WorldRenderer world_renderer;
    BackgroundRenderer background_renderer;

    LLGL::ResourceHeap* resource_heap = nullptr;

    LLGL::Buffer* chunk_vertex_buffer = nullptr;

    LLGL::PipelineState* postprocess_pipeline = nullptr;
    LLGL::Buffer* postprocess_vertex_buffer = nullptr;

    bool update_light = false;
} state;

uint32_t GameRenderer::GetMainOrderIndex() { return state.main_batch.order(); }
uint32_t GameRenderer::GetWorldOrderIndex() { return state.world_batch.order(); }
LLGL::Buffer* GameRenderer::ChunkVertexBuffer() { return state.chunk_vertex_buffer; }

bool GameRenderer::Init(const LLGL::Extent2D& resolution) {
    sge::Renderer& renderer = sge::Engine::Renderer();
    const auto& context = renderer.Context();
    const uint32_t samples = renderer.SwapChain()->GetSamples();

    const sge::Vertex vertices[] = {
        sge::Vertex(0.0, 0.0),
        sge::Vertex(0.0, 1.0),
        sge::Vertex(1.0, 0.0),
        sge::Vertex(1.0, 1.0),
    };

    state.chunk_vertex_buffer = renderer.CreateVertexBufferInit(sizeof(vertices), vertices, Assets::GetVertexFormat(VertexFormatAsset::TilemapVertex), "WorldRenderer VertexBuffer");

    state.world_renderer.init();
    state.background_renderer.init();

    ResizeTextures(resolution);

    LLGL::PipelineLayoutDescriptor pipelineLayoutDesc;
    pipelineLayoutDesc.staticSamplers = {
        LLGL::StaticSamplerDescriptor("u_background_sampler", LLGL::StageFlags::FragmentStage, 4, Assets::GetSampler(sge::TextureSampler::Nearest).descriptor()),
        LLGL::StaticSamplerDescriptor("u_world_sampler", LLGL::StageFlags::FragmentStage, 6, Assets::GetSampler(sge::TextureSampler::Nearest).descriptor()),
        LLGL::StaticSamplerDescriptor("u_lightmap_sampler", LLGL::StageFlags::FragmentStage, 8, Assets::GetSampler(sge::TextureSampler::Nearest).descriptor()),
        LLGL::StaticSamplerDescriptor("u_light_sampler", LLGL::StageFlags::FragmentStage, 10, Assets::GetSampler(sge::TextureSampler::Nearest).descriptor()),
    };
    pipelineLayoutDesc.combinedTextureSamplers = {
        LLGL::CombinedTextureSamplerDescriptor{ "u_background_texture", "u_background_texture", "u_background_sampler", 3 },
        LLGL::CombinedTextureSamplerDescriptor{ "u_world_texture", "u_world_texture", "u_world_sampler", 5 },
        LLGL::CombinedTextureSamplerDescriptor{ "u_lightmap_texture", "u_lightmap_texture", "u_lightmap_sampler", 7 },
        LLGL::CombinedTextureSamplerDescriptor{ "u_light_texture", "u_light_texture", "u_light_sampler", 9 },
    };
    pipelineLayoutDesc.heapBindings = sge::BindingLayout({
        sge::BindingLayoutItem::ConstantBuffer(2, "GlobalUniformBuffer", LLGL::StageFlags::VertexStage),
        sge::BindingLayoutItem::Texture(3, "u_background_texture", LLGL::StageFlags::FragmentStage),
        sge::BindingLayoutItem::Texture(5, "u_world_texture", LLGL::StageFlags::FragmentStage),
        sge::BindingLayoutItem::Texture(7, "u_lightmap_texture", LLGL::StageFlags::FragmentStage),
        sge::BindingLayoutItem::Texture(9, "u_light_texture", LLGL::StageFlags::FragmentStage),
    });

    LLGL::PipelineLayout* pipelineLayout = context->CreatePipelineLayout(pipelineLayoutDesc);

    const LLGL::ResourceViewDescriptor resource_views[] = {
        renderer.GlobalUniformBuffer(),
        state.background_renderer.target_texture(),
        state.world_renderer.target_texture(),
        state.world_renderer.static_lightmap_texture(),
        Assets::GetTexture(TextureAsset::Stub)
    };
    state.resource_heap = context->CreateResourceHeap(LLGL::ResourceHeapDescriptor(pipelineLayout, ARRAY_LEN(resource_views)), resource_views);

    const sge::ShaderPipeline& postprocess_shader = Assets::GetShader(ShaderAsset::PostProcessShader);

    LLGL::GraphicsPipelineDescriptor pipelineDesc;
    pipelineDesc.debugName = "LightMap Pipeline";
    pipelineDesc.vertexShader = postprocess_shader.vs;
    pipelineDesc.fragmentShader = postprocess_shader.ps;
    pipelineDesc.pipelineLayout = pipelineLayout;
    pipelineDesc.indexFormat = LLGL::Format::R16UInt;
    pipelineDesc.primitiveTopology = LLGL::PrimitiveTopology::TriangleStrip;
    pipelineDesc.rasterizer.frontCCW = true;
    pipelineDesc.rasterizer.multiSampleEnabled = (samples > 1);

    state.postprocess_pipeline = context->CreatePipelineState(pipelineDesc);

    state.background_renderer.init_world(state.world_renderer);
    state.particle_renderer.init();

    const sge::ShaderPipeline& font_shader = Assets::GetShader(ShaderAsset::FontShader);
    state.main_batch = sge::Batch(renderer, font_shader.ps);
    state.world_batch = sge::Batch(renderer, font_shader.ps);
    state.ui_batch = sge::Batch(renderer, font_shader.ps);

    state.world_batch.SetDepthEnabled(true);
    state.ui_batch.SetIsUi(true);

    return true;
}

void GameRenderer::ResizeTextures(LLGL::Extent2D size) {
    sge::Renderer& renderer = sge::Engine::Renderer();
    const auto& context = renderer.Context();
    const uint32_t samples = renderer.SwapChain()->GetSamples();

    const LLGL::Extent2D resolution = LLGL::Extent2D(size.width * samples, size.height * samples);

    state.world_renderer.init_targets(resolution);
    state.background_renderer.init_targets(resolution);

    if (state.resource_heap != nullptr) {
        context->WriteResourceHeap(*state.resource_heap, 1, {state.background_renderer.target_texture()});
        context->WriteResourceHeap(*state.resource_heap, 2, {state.world_renderer.target_texture()});
        context->WriteResourceHeap(*state.resource_heap, 3, {state.world_renderer.static_lightmap_texture()});
    }
}

void GameRenderer::InitWorldRenderer(const WorldData &world) {
    using Constants::SUBDIVISION;
    using Constants::TILE_SIZE;

    sge::Renderer& renderer = sge::Engine::Renderer();
    const auto& context = renderer.Context();

    SGE_RESOURCE_RELEASE(state.postprocess_vertex_buffer);

    const glm::vec2 world_size = glm::vec2(world.area.size()) * TILE_SIZE;

    const glm::vec2 vertices[] = {
        glm::vec2(-1.0f, 1.0f),  glm::vec2(0.0f, 0.0f), world_size,
        glm::vec2(3.0f,  1.0f),  glm::vec2(2.0f, 0.0f), world_size,
        glm::vec2(-1.0f, -3.0f), glm::vec2(0.0f, 2.0f), world_size,
    };
    state.postprocess_vertex_buffer = renderer.CreateVertexBufferInit(sizeof(vertices), vertices, Assets::GetVertexFormat(VertexFormatAsset::PostProcessVertex));

    state.world_renderer.init_textures(world);
    context->WriteResourceHeap(*state.resource_heap, 4, {state.world_renderer.light_texture()});

    state.world_renderer.init_lightmap_chunks(world);
    state.world_renderer.init_lighting(world);
}

void GameRenderer::UpdateLight() {
    state.update_light = true;
}

void GameRenderer::Begin(const sge::Camera& camera, World& world) {
    ZoneScoped;

    sge::Renderer& renderer = sge::Engine::Renderer();
    auto* const commands = renderer.CommandBuffer();
    auto* const command_queue = renderer.CommandQueue();

    const sge::Rect camera_frustum = sge::Rect::from_corners(
        camera.position() + camera.get_projection_area().min,
        camera.position() + camera.get_projection_area().max
    );
    const sge::Rect nozoom_camera_frustum = sge::Rect::from_corners(
        camera.position() + camera.get_nozoom_projection_area().min,
        camera.position() + camera.get_nozoom_projection_area().max
    );
    const sge::Rect ui_frustum = sge::Rect::from_corners(glm::vec2(0.0), camera.viewport());

    state.camera_frustums[CAMERA_FRUSTUM] = camera_frustum;
    state.camera_frustums[NOZOOM_CAMERA_FRUSTUM] = nozoom_camera_frustum;
    state.ui_frustum = ui_frustum;

    state.world_renderer.update(world);

    if (state.update_light) {
        commands->Begin();
            renderer.BeginPass(*state.world_renderer.light_texture_target());
                renderer.Clear(LLGL::ClearValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f), LLGL::ClearFlags::Color);
            renderer.EndPass();
        commands->End();
        command_queue->Submit(*commands);
    }

    renderer.Begin(camera);

    if (state.update_light) {
        state.world_renderer.compute_light(camera, world);
    }

    state.main_batch.Reset();
    state.world_batch.Reset();
    state.ui_batch.Reset();
}

void GameRenderer::Render(const sge::Camera& camera, const World& world) {
    ZoneScoped;

    sge::Renderer& renderer = sge::Engine::Renderer();
    auto* const commands = renderer.CommandBuffer();

    state.particle_renderer.compute();
    state.particle_renderer.prepare();

    LLGL::ClearValue clear_value = LLGL::ClearValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f);

    renderer.PrepareBatch(state.main_batch);
    renderer.PrepareBatch(state.world_batch);
    renderer.PrepareBatch(state.ui_batch);

    renderer.UploadBatchData();

    if (state.update_light) {
        renderer.BeginPass(*state.world_renderer.static_lightmap_target());
            renderer.Clear(clear_value, LLGL::ClearFlags::Color);
            state.world_renderer.render_lightmap(camera);
        renderer.EndPass();

        state.update_light = false;
    }

    // renderer.BeginPass(*state.background_renderer.target());
    //     renderer.Clear(clear_value, LLGL::ClearFlags::Color);
    //     state.background_renderer.render();
    // renderer.EndPass();

    renderer.BeginPass(*state.world_renderer.target());
        renderer.Clear(clear_value, LLGL::ClearFlags::ColorDepth);

        state.background_renderer.render();

        state.background_renderer.render_world();
        state.world_renderer.render(world.chunk_manager());
        state.particle_renderer.render_world();

        renderer.RenderBatch(state.world_batch);
    renderer.EndPass();

    renderer.BeginMainPass();
        renderer.Clear(clear_value);

        commands->SetVertexBuffer(*state.postprocess_vertex_buffer);
        commands->SetPipelineState(*state.postprocess_pipeline);
        commands->SetResourceHeap(*state.resource_heap);
        commands->Draw(3, 0);

        state.particle_renderer.render();

        renderer.RenderBatch(state.main_batch);
        renderer.RenderBatch(state.ui_batch);
    renderer.EndPass();

    renderer.End();

    state.particle_renderer.reset();
    state.background_renderer.reset();
}

void GameRenderer::BeginOrderMode(int order, bool advance) {
    state.main_batch.BeginOrderMode(order, advance);
    state.world_batch.BeginOrderMode(order, advance);
    state.ui_batch.BeginOrderMode(order, advance);
}

void GameRenderer::EndOrderMode() {
    state.main_batch.EndOrderMode();
    state.world_batch.EndOrderMode();
    state.ui_batch.EndOrderMode();
}

void GameRenderer::BeginBlendMode(sge::BlendMode blend_mode) {
    state.main_batch.BeginBlendMode(blend_mode);
    state.world_batch.BeginBlendMode(blend_mode);
    state.ui_batch.BeginBlendMode(blend_mode);
}

void GameRenderer::EndBlendMode() {
    state.main_batch.EndBlendMode();
    state.world_batch.EndBlendMode();
    state.ui_batch.EndBlendMode();
}

uint32_t GameRenderer::DrawSprite(const sge::Sprite& sprite, sge::Order order) {
    ZoneScoped;

    const sge::Rect aabb = sprite.calculate_aabb();
    if (!state.camera_frustums[sprite.ignore_camera_zoom()].intersects(aabb)) return 0;

    return state.main_batch.DrawSprite(sprite, order);
}

uint32_t GameRenderer::DrawSpriteWorld(const sge::Sprite& sprite, sge::Order order) {
    ZoneScoped;

    const sge::Rect aabb = sprite.calculate_aabb();
    if (!state.camera_frustums[sprite.ignore_camera_zoom()].intersects(aabb)) return 0;

    return state.world_batch.DrawSprite(sprite, order);
}

uint32_t GameRenderer::DrawSpriteUI(const sge::Sprite& sprite, sge::Order order) {
    ZoneScoped;

    const sge::Rect aabb = sprite.calculate_aabb();
    if (!state.ui_frustum.intersects(aabb)) return 0;

    return state.ui_batch.DrawSprite(sprite, order);
}

uint32_t GameRenderer::DrawAtlasSprite(const sge::TextureAtlasSprite& sprite, sge::Order order) {
    ZoneScoped;

    const sge::Rect aabb = sprite.calculate_aabb();
    if (!state.camera_frustums[sprite.ignore_camera_zoom()].intersects(aabb)) return 0;

    return state.main_batch.DrawAtlasSprite(sprite, order);
}

uint32_t GameRenderer::DrawAtlasSpriteWorld(const sge::TextureAtlasSprite& sprite, sge::Order order) {
    ZoneScoped;

    const sge::Rect aabb = sprite.calculate_aabb();
    if (!state.camera_frustums[sprite.ignore_camera_zoom()].intersects(aabb)) return 0;

    return state.world_batch.DrawAtlasSprite(sprite, order);
}

uint32_t GameRenderer::DrawAtlasSpriteWorldPremultiplied(const sge::TextureAtlasSprite& sprite, sge::Order order) {
    ZoneScoped;

    const sge::Rect aabb = sprite.calculate_aabb();
    if (!state.camera_frustums[sprite.ignore_camera_zoom()].intersects(aabb)) return 0;

    state.world_batch.BeginBlendMode(sge::BlendMode::PremultipliedAlpha);
    uint32_t ordr = state.world_batch.DrawAtlasSprite(sprite, order);
    state.world_batch.EndBlendMode();

    return ordr;
}

uint32_t GameRenderer::DrawAtlasSpriteUI(const sge::TextureAtlasSprite& sprite, sge::Order order) {
    ZoneScoped;

    const sge::Rect aabb = sprite.calculate_aabb();
    if (!state.ui_frustum.intersects(aabb)) return 0;

    return state.ui_batch.DrawAtlasSprite(sprite, order);
}

uint32_t GameRenderer::DrawNinePatchUI(const sge::NinePatch& ninepatch, sge::Order order) {
    ZoneScoped;

    const sge::Rect aabb = ninepatch.calculate_aabb();
    if (!state.ui_frustum.intersects(aabb)) return 0;

    return state.ui_batch.DrawNinePatch(ninepatch, order);
}

uint32_t GameRenderer::DrawText(const sge::RichTextSection* sections, size_t size, const glm::vec2& position, const sge::Font& font, sge::Order order) {
    ZoneScoped;

    return state.main_batch.DrawText(sections, size, position, font, order);
}

uint32_t GameRenderer::DrawTextUI(const sge::RichTextSection* sections, size_t size, const glm::vec2& position, const sge::Font& font, sge::Order order) {
    ZoneScoped;

    return state.ui_batch.DrawText(sections, size, position, font, order);
}

void GameRenderer::DrawBackground(const BackgroundLayer& layer) {
    ZoneScoped;

    const sge::Rect aabb = sge::Rect::from_top_left(layer.position() - layer.anchor().to_vec2() * layer.size(), layer.size());
    if (!state.camera_frustums[layer.nonscale()].intersects(aabb)) return;

    if (layer.is_world()) {
        state.background_renderer.draw_world_layer(layer);
    } else {
        state.background_renderer.draw_layer(layer);
    }
}

void GameRenderer::DrawParticle(const glm::vec2& position, const glm::quat& rotation, float scale, Particle::Type type, uint8_t variant, sge::Order order, bool world) {
    ZoneScoped;

    if (world)
        state.particle_renderer.draw_particle_world(position, rotation, scale, type, variant, order);
    else
        state.particle_renderer.draw_particle(position, rotation, scale, type, variant, order);
}

void GameRenderer::Terminate() {
    state.world_renderer.terminate();
    state.background_renderer.terminate();
    state.particle_renderer.terminate();

    const auto& context = sge::Engine::Renderer().Context();

    state.main_batch.Terminate(context);
    state.world_batch.Terminate(context);
    state.ui_batch.Terminate(context);

    SGE_RESOURCE_RELEASE(state.resource_heap);
    SGE_RESOURCE_RELEASE(state.chunk_vertex_buffer);
    SGE_RESOURCE_RELEASE(state.postprocess_pipeline);
    SGE_RESOURCE_RELEASE(state.postprocess_vertex_buffer);
}