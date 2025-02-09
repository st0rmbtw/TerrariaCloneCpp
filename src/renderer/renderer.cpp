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

#include "../assets.hpp"
#include "../engine/engine.hpp"
#include "../engine/renderer/macros.hpp"
#include "../utils.hpp"

#include "world_renderer.hpp"
#include "background_renderer.hpp"
#include "particle_renderer.hpp"

static constexpr int CAMERA_FRUSTUM = 0;
static constexpr int NOZOOM_CAMERA_FRUSTUM = 1;

static struct RendererState {
    Batch main_batch;
    Batch world_batch;
    Batch ui_batch;

    math::Rect camera_frustums[2];
    math::Rect ui_frustum;

    ParticleRenderer particle_renderer;
    BackgroundRenderer background_renderer;
    WorldRenderer world_renderer;

    Renderer* renderer = nullptr;

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

    bool update_light = false;
} state;

uint32_t GameRenderer::GetMainOrderIndex() { return state.main_batch.order(); }
uint32_t GameRenderer::GetWorldOrderIndex() { return state.world_batch.order(); }
LLGL::Buffer* GameRenderer::ChunkVertexBuffer() { return state.chunk_vertex_buffer; }

bool GameRenderer::Init(const LLGL::Extent2D& resolution) {
    state.renderer = &Engine::Renderer();

    ResizeTextures(resolution);

    const auto& context = state.renderer->Context();

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

    state.chunk_vertex_buffer = state.renderer->CreateVertexBufferInit(sizeof(vertices), vertices, Assets::GetVertexFormat(VertexFormatAsset::TilemapVertex), "WorldRenderer VertexBuffer");

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
        state.renderer->GlobalUniformBuffer(),
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

    state.world_renderer.init(state.static_lightmap_render_pass);
    state.background_renderer.init();
    state.particle_renderer.init();

    state.world_batch.set_depth_enabled(true);
    state.ui_batch.set_is_ui(true);

    return true;
}

void GameRenderer::ResizeTextures(LLGL::Extent2D resolution) {
    const auto& context = state.renderer->Context();
    const auto* swap_chain = state.renderer->SwapChain();

    RESOURCE_RELEASE(state.world_render_target);
    RESOURCE_RELEASE(state.world_render_texture);
    RESOURCE_RELEASE(state.world_depth_texture);

    RESOURCE_RELEASE(state.background_render_target);
    RESOURCE_RELEASE(state.background_render_texture);

    RESOURCE_RELEASE(state.static_lightmap_target);
    RESOURCE_RELEASE(state.static_lightmap_texture);

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

void GameRenderer::InitWorldRenderer(const WorldData &world) {
    using Constants::SUBDIVISION;
    using Constants::TILE_SIZE;

    const auto& context = state.renderer->Context();

    RESOURCE_RELEASE(state.fullscreen_triangle_vertex_buffer);

    const glm::vec2 world_size = glm::vec2(world.area.size()) * TILE_SIZE;

    const glm::vec2 vertices[] = {
        glm::vec2(-1.0f, 1.0f),  glm::vec2(0.0f, 0.0f), world_size,
        glm::vec2(3.0f,  1.0f),  glm::vec2(2.0f, 0.0f), world_size,
        glm::vec2(-1.0f, -3.0f), glm::vec2(0.0f, 2.0f), world_size,
    };
    state.fullscreen_triangle_vertex_buffer = state.renderer->CreateVertexBufferInit(sizeof(vertices), vertices, Assets::GetVertexFormat(VertexFormatAsset::PostProcessVertex));

    state.world_renderer.init_textures(world);
    context->WriteResourceHeap(*state.resource_heap, 4, {state.world_renderer.light_texture()});

    state.world_renderer.init_lightmap_chunks(world);
}

void GameRenderer::UpdateLight() {
    state.update_light = true;
}

void GameRenderer::Begin(const Camera& camera, WorldData& world) {
    ZoneScopedN("Renderer::Begin");

    const math::Rect camera_frustum = math::Rect::from_corners(
        camera.position() + camera.get_projection_area().min,
        camera.position() + camera.get_projection_area().max
    );
    const math::Rect nozoom_camera_frustum = math::Rect::from_corners(
        camera.position() + camera.get_nozoom_projection_area().min,
        camera.position() + camera.get_nozoom_projection_area().max
    );
    const math::Rect ui_frustum = math::Rect::from_corners(glm::vec2(0.0), camera.viewport());

    state.camera_frustums[CAMERA_FRUSTUM] = camera_frustum;
    state.camera_frustums[NOZOOM_CAMERA_FRUSTUM] = nozoom_camera_frustum;
    state.ui_frustum = ui_frustum;

    state.world_renderer.update_lightmap_texture(world);
    state.world_renderer.update_tile_texture(world);

    state.renderer->Begin(camera);

    state.main_batch.Reset();
    state.world_batch.Reset();
    state.ui_batch.Reset();
}

void GameRenderer::Render(const Camera& camera, const World& world) {
    ZoneScopedN("Renderer::Render");

    auto* const commands = state.renderer->CommandBuffer();
    auto* const swap_chain = state.renderer->SwapChain();

    state.particle_renderer.compute();
    state.particle_renderer.prepare();

    LLGL::ClearValue clear_value = LLGL::ClearValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f);

    state.renderer->PrepareBatch(state.main_batch);
    state.renderer->PrepareBatch(state.world_batch);
    state.renderer->PrepareBatch(state.ui_batch);

    state.renderer->UploadBatchData();

    if (state.update_light) {
        commands->BeginRenderPass(*state.world_renderer.light_texture_target());
            commands->Clear(LLGL::ClearFlags::Color, clear_value);
        commands->EndRenderPass();

        state.world_renderer.compute_light(camera, world);

        commands->BeginRenderPass(*state.static_lightmap_target);
            commands->Clear(LLGL::ClearFlags::Color, clear_value);
            state.world_renderer.render_lightmap(camera);
        commands->EndRenderPass();

        state.update_light = false;
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

        state.renderer->RenderBatch(state.world_batch);

    commands->EndRenderPass();

    commands->BeginRenderPass(*swap_chain);
        commands->Clear(LLGL::ClearFlags::Color, LLGL::ClearValue(1.0f, 0.0f, 0.0f, 1.0f, 0.0f));

        commands->SetVertexBuffer(*state.fullscreen_triangle_vertex_buffer);
        commands->SetPipelineState(*state.postprocess_pipeline);
        commands->SetResourceHeap(*state.resource_heap);

        commands->Draw(3, 0);

        state.particle_renderer.render();

        state.renderer->RenderBatch(state.main_batch);
        state.renderer->RenderBatch(state.ui_batch);
    commands->EndRenderPass();

    state.renderer->End();

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

uint32_t GameRenderer::DrawSprite(const Sprite& sprite, Order order) {
    ZoneScopedN("Renderer::DrawSprite");

    const math::Rect aabb = sprite.calculate_aabb();
    if (!state.camera_frustums[sprite.ignore_camera_zoom()].intersects(aabb)) return 0;

    return state.main_batch.DrawSprite(sprite, order);
}

uint32_t GameRenderer::DrawSpriteWorld(const Sprite& sprite, Order order) {
    ZoneScopedN("Renderer::DrawSprite");

    const math::Rect aabb = sprite.calculate_aabb();
    if (!state.camera_frustums[sprite.ignore_camera_zoom()].intersects(aabb)) return 0;

    return state.world_batch.DrawSprite(sprite, order);
}

uint32_t GameRenderer::DrawSpriteUI(const Sprite& sprite, Order order) {
    ZoneScopedN("Renderer::DrawSpriteUI");

    const math::Rect aabb = sprite.calculate_aabb();
    if (!state.ui_frustum.intersects(aabb)) return 0;

    return state.ui_batch.DrawSprite(sprite, order);
}

uint32_t GameRenderer::DrawAtlasSprite(const TextureAtlasSprite& sprite, Order order) {
    ZoneScopedN("Renderer::DrawAtlasSprite");

    const math::Rect aabb = sprite.calculate_aabb();
    if (!state.camera_frustums[sprite.ignore_camera_zoom()].intersects(aabb)) return 0;

    return state.main_batch.DrawAtlasSprite(sprite, order);
}

uint32_t GameRenderer::DrawAtlasSpriteWorld(const TextureAtlasSprite& sprite, Order order) {
    ZoneScopedN("Renderer::DrawAtlasSprite");

    const math::Rect aabb = sprite.calculate_aabb();
    if (!state.camera_frustums[sprite.ignore_camera_zoom()].intersects(aabb)) return 0;

    return state.world_batch.DrawAtlasSprite(sprite, order);
}

uint32_t GameRenderer::DrawAtlasSpriteUI(const TextureAtlasSprite& sprite, Order order) {
    ZoneScopedN("Renderer::DrawAtlasSpriteUI");

    const math::Rect aabb = sprite.calculate_aabb();
    if (!state.ui_frustum.intersects(aabb)) return 0;

    return state.ui_batch.DrawAtlasSprite(sprite, order);
}

uint32_t GameRenderer::DrawNinePatchUI(const NinePatch& ninepatch, Order order) {
    ZoneScopedN("Renderer::DrawNinePatchUI");

    const math::Rect aabb = ninepatch.calculate_aabb();
    if (!state.ui_frustum.intersects(aabb)) return 0;

    return state.ui_batch.DrawNinePatch(ninepatch, order);
}

uint32_t GameRenderer::DrawText(const RichTextSection* sections, size_t size, const glm::vec2& position, FontAsset key, Order order) {
    ZoneScopedN("Renderer::DrawText");

    return state.main_batch.DrawText(sections, size, position, key, order);
}

uint32_t GameRenderer::DrawTextUI(const RichTextSection* sections, size_t size, const glm::vec2& position, FontAsset key, Order order) {
    ZoneScopedN("Renderer::DrawTextUI");

    return state.ui_batch.DrawText(sections, size, position, key, order);
}

void GameRenderer::DrawBackground(const BackgroundLayer& layer) {
    ZoneScopedN("Renderer::DrawBackground");

    const math::Rect aabb = math::Rect::from_top_left(layer.position() - layer.anchor().to_vec2() * layer.size(), layer.size());
    if (!state.camera_frustums[layer.nonscale()].intersects(aabb)) return;

    if (layer.is_world()) {
        state.background_renderer.draw_world_layer(layer);
    } else {
        state.background_renderer.draw_layer(layer);
    }
}

void GameRenderer::DrawParticle(const glm::vec2& position, const glm::quat& rotation, float scale, Particle::Type type, uint8_t variant, Order order, bool world) {
    ZoneScopedN("Renderer::DrawParticle");

    if (world)
        state.particle_renderer.draw_particle_world(position, rotation, scale, type, variant, order);
    else
        state.particle_renderer.draw_particle(position, rotation, scale, type, variant, order);
}
