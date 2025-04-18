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

#include <tracy/Tracy.hpp>

#include <SGE/engine.hpp>
#include <SGE/renderer/macros.hpp>
#include <SGE/renderer/batch.hpp>

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
    BackgroundRenderer background_renderer;
    WorldRenderer world_renderer;

    LLGL::ResourceHeap* resource_heap = nullptr;

    LLGL::Buffer* chunk_vertex_buffer = nullptr;

    LLGL::Texture* background_render_texture = nullptr;
    LLGL::RenderTarget* background_render_target = nullptr;

    LLGL::Texture* static_lightmap_texture = nullptr;
    LLGL::RenderTarget* static_lightmap_target = nullptr;
    LLGL::RenderPass* static_lightmap_render_pass = nullptr;

    LLGL::PipelineState* postprocess_pipeline = nullptr;
    LLGL::Buffer* fullscreen_triangle_vertex_buffer = nullptr;

    bool update_light = false;
} state;

uint32_t GameRenderer::GetMainOrderIndex() { return state.main_batch.order(); }
uint32_t GameRenderer::GetWorldOrderIndex() { return state.world_batch.order(); }
LLGL::Buffer* GameRenderer::ChunkVertexBuffer() { return state.chunk_vertex_buffer; }

bool GameRenderer::Init(const LLGL::Extent2D& resolution) {
    sge::Renderer& renderer = sge::Engine::Renderer();
    const auto& context = renderer.Context();
    const LLGL::SwapChain* swap_chain = renderer.SwapChain();

    const sge::Vertex vertices[] = {
        sge::Vertex(0.0, 0.0),
        sge::Vertex(0.0, 1.0),
        sge::Vertex(1.0, 0.0),
        sge::Vertex(1.0, 1.0),
    };

    state.chunk_vertex_buffer = renderer.CreateVertexBufferInit(sizeof(vertices), vertices, Assets::GetVertexFormat(VertexFormatAsset::TilemapVertex), "WorldRenderer VertexBuffer");

    {
        LLGL::RenderPassDescriptor static_lightmap_render_pass_desc;
        static_lightmap_render_pass_desc.colorAttachments[0].loadOp = LLGL::AttachmentLoadOp::Undefined;
        static_lightmap_render_pass_desc.colorAttachments[0].storeOp = LLGL::AttachmentStoreOp::Store;
        static_lightmap_render_pass_desc.colorAttachments[0].format = swap_chain->GetColorFormat();
        state.static_lightmap_render_pass = context->CreateRenderPass(static_lightmap_render_pass_desc);
    }

    state.world_renderer.init(state.static_lightmap_render_pass);

    ResizeTextures(resolution);

    LLGL::PipelineLayoutDescriptor pipelineLayoutDesc;
    pipelineLayoutDesc.staticSamplers = {   
        LLGL::StaticSamplerDescriptor("u_background_sampler", LLGL::StageFlags::FragmentStage, 4, Assets::GetSampler(sge::TextureSampler::Nearest).descriptor()),
        LLGL::StaticSamplerDescriptor("u_world_sampler", LLGL::StageFlags::FragmentStage, 6, Assets::GetSampler(sge::TextureSampler::Nearest).descriptor()),
        LLGL::StaticSamplerDescriptor("u_lightmap_sampler", LLGL::StageFlags::FragmentStage, 8, Assets::GetSampler(sge::TextureSampler::Nearest).descriptor()),
        LLGL::StaticSamplerDescriptor("u_light_sampler", LLGL::StageFlags::FragmentStage, 10, Assets::GetSampler(sge::TextureSampler::Nearest).descriptor()),
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
        renderer.GlobalUniformBuffer(),
        state.background_render_texture,
        state.world_renderer.target_texture(),
        state.static_lightmap_texture,
        nullptr,
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

    state.postprocess_pipeline = context->CreatePipelineState(pipelineDesc);

    state.background_renderer.init();
    state.background_renderer.init_world(state.world_renderer);
    state.particle_renderer.init();

    state.world_batch.set_depth_enabled(true);
    state.ui_batch.set_is_ui(true);

    return true;
}

void GameRenderer::ResizeTextures(LLGL::Extent2D resolution) {
    sge::Renderer& renderer = sge::Engine::Renderer();
    const auto& context = renderer.Context();
    const auto* swap_chain = renderer.SwapChain();

    SGE_RESOURCE_RELEASE(state.background_render_target);
    SGE_RESOURCE_RELEASE(state.background_render_texture);

    SGE_RESOURCE_RELEASE(state.static_lightmap_target);
    SGE_RESOURCE_RELEASE(state.static_lightmap_texture);

    state.world_renderer.init_target(resolution);

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

    LLGL::Texture* background_render_texture = context->CreateTexture(texture_desc);
    LLGL::Texture* static_lightmap_texture = context->CreateTexture(texture_desc);

    state.background_render_texture = background_render_texture;
    state.static_lightmap_texture = static_lightmap_texture;
    
    LLGL::RenderTargetDescriptor background_target_desc;
    background_target_desc.resolution = resolution;
    background_target_desc.colorAttachments[0] = background_render_texture;
    state.background_render_target = context->CreateRenderTarget(background_target_desc);

    LLGL::RenderTargetDescriptor static_lightmap_target_desc;
    static_lightmap_target_desc.renderPass = state.static_lightmap_render_pass;
    static_lightmap_target_desc.resolution = resolution;
    static_lightmap_target_desc.colorAttachments[0] = static_lightmap_texture;
    state.static_lightmap_target = context->CreateRenderTarget(static_lightmap_target_desc);

    if (state.resource_heap != nullptr) {
        context->WriteResourceHeap(*state.resource_heap, 1, {background_render_texture});
        context->WriteResourceHeap(*state.resource_heap, 2, {state.world_renderer.target_texture()});
        context->WriteResourceHeap(*state.resource_heap, 3, {static_lightmap_texture});
    }
}

void GameRenderer::InitWorldRenderer(const WorldData &world) {
    using Constants::SUBDIVISION;
    using Constants::TILE_SIZE;

    sge::Renderer& renderer = sge::Engine::Renderer();
    const auto& context = renderer.Context();

    SGE_RESOURCE_RELEASE(state.fullscreen_triangle_vertex_buffer);

    const glm::vec2 world_size = glm::vec2(world.area.size()) * TILE_SIZE;

    const glm::vec2 vertices[] = {
        glm::vec2(-1.0f, 1.0f),  glm::vec2(0.0f, 0.0f), world_size,
        glm::vec2(3.0f,  1.0f),  glm::vec2(2.0f, 0.0f), world_size,
        glm::vec2(-1.0f, -3.0f), glm::vec2(0.0f, 2.0f), world_size,
    };
    state.fullscreen_triangle_vertex_buffer = renderer.CreateVertexBufferInit(sizeof(vertices), vertices, Assets::GetVertexFormat(VertexFormatAsset::PostProcessVertex));

    state.world_renderer.init_textures(world);
    context->WriteResourceHeap(*state.resource_heap, 4, {state.world_renderer.light_texture()});

    state.world_renderer.init_lightmap_chunks(world);
}

void GameRenderer::UpdateLight() {
    state.update_light = true;
}

void GameRenderer::Begin(const sge::Camera& camera, WorldData& world) {
    ZoneScopedN("Renderer::Begin");

    sge::Renderer& renderer = sge::Engine::Renderer();

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

    state.world_renderer.update_lightmap_texture(world);
    state.world_renderer.update_tile_texture(world);

    renderer.Begin(camera);

    state.main_batch.Reset();
    state.world_batch.Reset();
    state.ui_batch.Reset();
}

void GameRenderer::Render(const sge::Camera& camera, const World& world) {
    ZoneScopedN("Renderer::Render");

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
        renderer.BeginPass(*state.world_renderer.light_texture_target(), clear_value, LLGL::ClearFlags::Color);
        renderer.EndPass();

        state.world_renderer.compute_light(camera, world);

        renderer.BeginPass(*state.static_lightmap_target, clear_value, LLGL::ClearFlags::Color);
            state.world_renderer.render_lightmap(camera);
        renderer.EndPass();

        state.update_light = false;
    }

    renderer.BeginPass(*state.background_render_target, clear_value, LLGL::ClearFlags::ColorDepth);
        state.background_renderer.render();
    renderer.EndPass();

    renderer.BeginPass(*state.world_renderer.target(), clear_value, LLGL::ClearFlags::ColorDepth);
        state.background_renderer.render_world();
        state.world_renderer.render(world.chunk_manager());
        state.particle_renderer.render_world();

        renderer.RenderBatch(state.world_batch);
    renderer.EndPass();

    renderer.BeginMainPass(LLGL::ClearValue(1.0f, 0.0f, 0.0f, 1.0f, 0.0f));
        commands->SetVertexBuffer(*state.fullscreen_triangle_vertex_buffer);
        commands->SetPipelineState(*state.postprocess_pipeline);
        commands->SetResourceHeap(*state.resource_heap);

        commands->Draw(3, 0);

        state.particle_renderer.render();

        renderer.RenderBatch(state.main_batch);
        renderer.RenderBatch(state.ui_batch);
    renderer.EndPass();

    renderer.End();

    state.particle_renderer.reset();
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

uint32_t GameRenderer::DrawSprite(const sge::Sprite& sprite, sge::Order order) {
    ZoneScopedN("Renderer::DrawSprite");

    const sge::Rect aabb = sprite.calculate_aabb();
    if (!state.camera_frustums[sprite.ignore_camera_zoom()].intersects(aabb)) return 0;

    return state.main_batch.DrawSprite(sprite, order);
}

uint32_t GameRenderer::DrawSpriteWorld(const sge::Sprite& sprite, sge::Order order) {
    ZoneScopedN("Renderer::DrawSprite");

    const sge::Rect aabb = sprite.calculate_aabb();
    if (!state.camera_frustums[sprite.ignore_camera_zoom()].intersects(aabb)) return 0;

    return state.world_batch.DrawSprite(sprite, order);
}

uint32_t GameRenderer::DrawSpriteUI(const sge::Sprite& sprite, sge::Order order) {
    ZoneScopedN("Renderer::DrawSpriteUI");

    const sge::Rect aabb = sprite.calculate_aabb();
    if (!state.ui_frustum.intersects(aabb)) return 0;

    return state.ui_batch.DrawSprite(sprite, order);
}

uint32_t GameRenderer::DrawAtlasSprite(const sge::TextureAtlasSprite& sprite, sge::Order order) {
    ZoneScopedN("Renderer::DrawAtlasSprite");

    const sge::Rect aabb = sprite.calculate_aabb();
    if (!state.camera_frustums[sprite.ignore_camera_zoom()].intersects(aabb)) return 0;

    return state.main_batch.DrawAtlasSprite(sprite, order);
}

uint32_t GameRenderer::DrawAtlasSpriteWorld(const sge::TextureAtlasSprite& sprite, sge::Order order) {
    ZoneScopedN("Renderer::DrawAtlasSprite");

    const sge::Rect aabb = sprite.calculate_aabb();
    if (!state.camera_frustums[sprite.ignore_camera_zoom()].intersects(aabb)) return 0;

    return state.world_batch.DrawAtlasSprite(sprite, order);
}

uint32_t GameRenderer::DrawAtlasSpriteUI(const sge::TextureAtlasSprite& sprite, sge::Order order) {
    ZoneScopedN("Renderer::DrawAtlasSpriteUI");

    const sge::Rect aabb = sprite.calculate_aabb();
    if (!state.ui_frustum.intersects(aabb)) return 0;

    return state.ui_batch.DrawAtlasSprite(sprite, order);
}

uint32_t GameRenderer::DrawNinePatchUI(const sge::NinePatch& ninepatch, sge::Order order) {
    ZoneScopedN("Renderer::DrawNinePatchUI");

    const sge::Rect aabb = ninepatch.calculate_aabb();
    if (!state.ui_frustum.intersects(aabb)) return 0;

    return state.ui_batch.DrawNinePatch(ninepatch, order);
}

uint32_t GameRenderer::DrawText(const sge::RichTextSection* sections, size_t size, const glm::vec2& position, const sge::Font& font, sge::Order order) {
    ZoneScopedN("Renderer::DrawText");

    return state.main_batch.DrawText(sections, size, position, font, order);
}

uint32_t GameRenderer::DrawTextUI(const sge::RichTextSection* sections, size_t size, const glm::vec2& position, const sge::Font& font, sge::Order order) {
    ZoneScopedN("Renderer::DrawTextUI");

    return state.ui_batch.DrawText(sections, size, position, font, order);
}

void GameRenderer::DrawBackground(const BackgroundLayer& layer) {
    ZoneScopedN("Renderer::DrawBackground");

    const sge::Rect aabb = sge::Rect::from_top_left(layer.position() - layer.anchor().to_vec2() * layer.size(), layer.size());
    if (!state.camera_frustums[layer.nonscale()].intersects(aabb)) return;

    if (layer.is_world()) {
        state.background_renderer.draw_world_layer(layer);
    } else {
        state.background_renderer.draw_layer(layer);
    }
}

void GameRenderer::DrawParticle(const glm::vec2& position, const glm::quat& rotation, float scale, Particle::Type type, uint8_t variant, sge::Order order, bool world) {
    ZoneScopedN("Renderer::DrawParticle");

    if (world)
        state.particle_renderer.draw_particle_world(position, rotation, scale, type, variant, order);
    else
        state.particle_renderer.draw_particle(position, rotation, scale, type, variant, order);
}
