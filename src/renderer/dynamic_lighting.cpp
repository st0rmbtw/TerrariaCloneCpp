#include <tracy/Tracy.hpp>

#include <SGE/engine.hpp>
#include <SGE/renderer/macros.hpp>
#include <SGE/types/binding_layout.hpp>

#include "dynamic_lighting.hpp"

static bool blur(LightMap& lightmap, int index, glm::vec3& prev_light, float& prev_decay) {
    using Constants::LIGHT_EPSILON;

    glm::vec3 this_light = lightmap.get_color(index);

    prev_light.r = prev_light.r < LIGHT_EPSILON ? 0.0f : prev_light.r;
    prev_light.g = prev_light.g < LIGHT_EPSILON ? 0.0f : prev_light.g;
    prev_light.b = prev_light.b < LIGHT_EPSILON ? 0.0f : prev_light.b;

    if (prev_light.r < this_light.r) {
        prev_light.r = this_light.r;
    } else {
        this_light.r = prev_light.r;
    }

    if (prev_light.g < this_light.g) {
        prev_light.g = this_light.g;
    } else {
        this_light.g = prev_light.g;
    }

    if (prev_light.b < this_light.b) {
        prev_light.b = this_light.b;
    } else {
        this_light.b = prev_light.b;
    }

    lightmap.set_color(index, this_light);

    prev_light = prev_light * prev_decay;
    prev_decay = Constants::LightDecay(lightmap.get_mask(index));

    // The light has completely decayed - nothing to blur 
    return this_light.r < LIGHT_EPSILON && this_light.g < LIGHT_EPSILON && this_light.b < LIGHT_EPSILON;
}

SGE_FORCE_INLINE static void blur_line_fwd(LightMap& lightmap, int start, int end, int stride, glm::vec3& prev_light, float& prev_decay) {
    using Constants::LIGHT_EPSILON;

    int length = end - start;
    for (int index = 0; index < length; index += stride) {
        if (blur(lightmap, start + index, prev_light, prev_decay)) return;
    }
}

SGE_FORCE_INLINE static void blur_line_bwd(LightMap& lightmap, int start, int end, int stride, glm::vec3& prev_light, float& prev_decay) {
    using Constants::LIGHT_EPSILON;

    int length = start - end;
    for (int index = 0; index < length; index += stride) {
        if (blur(lightmap, start - index, prev_light, prev_decay)) return;
    }
}

DynamicLighting::DynamicLighting(const WorldData& world, LLGL::Texture* light_texture) : m_light_texture(light_texture) {
    m_dynamic_lightmap = LightMap(world.area.width(), world.area.height());
    m_renderer = &sge::Engine::Renderer();

    for (int y = 0; y < m_dynamic_lightmap.height; ++y) {
        for (int x = 0; x < m_dynamic_lightmap.width; ++x) {
            const TilePos color_pos = TilePos(x, y);
            const TilePos tile_pos = color_pos / Constants::SUBDIVISION;

            m_dynamic_lightmap.set_mask(color_pos, world.tile_exists(tile_pos));
        }
    }
}

void DynamicLighting::compute_light(const sge::Camera&, const World& world) {
    const uint32_t light_count = world.light_count();
    if (light_count == 0) return;

    m_areas.clear();

    LightMap& lightmap = m_dynamic_lightmap;

    sge::IRect process_area = sge::IRect::from_center_half_size(
        glm::ivec2(world.lights()[0].pos.x, world.lights()[0].pos.y), glm::ivec2(Constants::LIGHT_DECAY_STEPS)
    );

    for (size_t i = 1; i < light_count; ++i) {
        const Light& light = world.lights()[i];
        
        sge::IRect area = sge::IRect::from_center_half_size(
            glm::ivec2(light.pos.x, light.pos.y), glm::ivec2(Constants::LIGHT_DECAY_STEPS)
        );

        // if (area.intersects(process_area)) {
        //     process_area = process_area.merge(area);
        // } else {
            m_areas.push_back(area.clamp(sge::IRect({0, 0}, {lightmap.width, lightmap.height})));
            // process_area = area;
        // }
    }
    
    m_areas.push_back(process_area);
    process_area = process_area.clamp(sge::IRect({0, 0}, {lightmap.width, lightmap.height}));

    for (sge::IRect area : m_areas) {
        for (int y = area.min.y; y < area.max.y; ++y) {
            memset(&lightmap.colors[y * lightmap.width + area.min.x], 0, area.width() * sizeof(Color));
        }
    }

    for (size_t i = 0; i < light_count; ++i) {
        const Light& light = world.lights()[i];
        lightmap.set_color(light.pos, light.color);
    }

    for (sge::IRect area : m_areas) {
        const glm::ivec2 center = area.center();

        for (size_t i = 0; i < 2; ++i) {
            // Horizontal
            // m_thread_pool.start();
            for (int y = 0; y < area.height(); ++y) {
                // m_thread_pool.queue_job([&lightmap, y, center, &area] {
                    glm::vec3 prev_light = glm::vec3(0.0f);

                    float prev_decay = Constants::LightDecay(lightmap.get_mask({center.x + 1, y}));
                    blur_line_bwd(lightmap, (area.min.y + y) * lightmap.width + center.x, (area.min.y + y) * lightmap.width, 1, prev_light, prev_decay);

                    prev_decay = Constants::LightDecay(lightmap.get_mask({center.x - 1, y}));
                    blur_line_fwd(lightmap, (area.min.y + y) * lightmap.width + center.x, (area.min.y + y) * lightmap.width + area.max.x - 1, 1, prev_light, prev_decay);
                // });
            }
            // m_thread_pool.stop();

            // Vertical
            // m_thread_pool.start();
            for (int x = 0; x < area.width(); ++x) {
                // m_thread_pool.queue_job([&lightmap, x, center, &area] {
                    glm::vec3 prev_light = glm::vec3(0.0f);
                    
                    float prev_decay = Constants::LightDecay(lightmap.get_mask({x, center.y + 1}));
                    blur_line_bwd(lightmap, center.y * lightmap.width + area.min.x + x, area.min.y * lightmap.width + area.min.x + x, lightmap.width, prev_light, prev_decay);

                    prev_decay = Constants::LightDecay(lightmap.get_mask({x, center.y - 1}));
                    blur_line_fwd(lightmap, center.y * lightmap.width + area.min.x + x, (area.max.y - 1) * lightmap.width + area.min.x + x, lightmap.width, prev_light, prev_decay);
                // });
            }
            // m_thread_pool.stop();
        }

        // Horizontal
        // m_thread_pool.start();
        for (int y = 0; y < area.height(); ++y) {
            // m_thread_pool.queue_job([&lightmap, y, center, &area] {
                glm::vec3 prev_light = glm::vec3(0.0f);

                float prev_decay = Constants::LightDecay(lightmap.get_mask({center.x + 1, y}));
                blur_line_bwd(lightmap, (area.min.y + y) * lightmap.width + center.x, (area.min.y + y) * lightmap.width, 1, prev_light, prev_decay);

                prev_decay = Constants::LightDecay(lightmap.get_mask({center.x - 1, y}));
                blur_line_fwd(lightmap, (area.min.y + y) * lightmap.width + center.x, (area.min.y + y) * lightmap.width + area.max.x - 1, 1, prev_light, prev_decay);
            // });
        }
        // m_thread_pool.stop();

        const auto& context = m_renderer->Context();

        LLGL::ImageView image_view;
        image_view.format   = LLGL::ImageFormat::RGBA;
        image_view.dataType = LLGL::DataType::UInt8;
        image_view.data     = &lightmap.colors[area.min.y * lightmap.width + area.min.x];
        image_view.dataSize = area.width() * area.height() * sizeof(Color);
        image_view.rowStride = lightmap.width * sizeof(Color);
        context->WriteTexture(*m_light_texture, LLGL::TextureRegion(LLGL::Offset3D(area.min.x, area.min.y, 0), LLGL::Extent3D(area.width(), area.height(), 1)), image_view);

    }
}


// -------------------- AcceleratedDynamicLighting --------------------

static SGE_FORCE_INLINE void blur_dispatch(LLGL::CommandBuffer* commands, uint32_t width) {
    commands->Dispatch(width, 1, 1);
}

static SGE_FORCE_INLINE void blur_dispatch_metal(LLGL::CommandBuffer* commands, uint32_t width) {
    const uint32_t h = (width + 512 - 1) / 512;
    const uint32_t w = std::min(512u, width);
    commands->Dispatch(w, h, 1);
}

AcceleratedDynamicLighting::AcceleratedDynamicLighting(const WorldData& world, LLGL::Texture* light_texture) :
    m_light_texture(light_texture)
{
    using Constants::TILE_SIZE;

    m_renderer = &sge::Engine::Renderer();

    const sge::RenderBackend backend = m_renderer->Backend();

    if (backend.IsMetal()) {
        m_workgroup_size = 1;
        m_dispatch_func = blur_dispatch_metal;
    } else {
        m_dispatch_func = blur_dispatch;
    }

    init_pipeline();
    init_textures(world);
}

void AcceleratedDynamicLighting::init_pipeline() {
    const auto& context = m_renderer->Context();
    const sge::RenderBackend backend = m_renderer->Backend();

    LLGL::BufferDescriptor light_buffer;
    light_buffer.bindFlags = LLGL::BindFlags::Sampled;
    light_buffer.stride = sizeof(Light);
    light_buffer.size = sizeof(Light) * WORLD_MAX_LIGHT_COUNT;
    m_light_buffer = context->CreateBuffer(light_buffer);

    {
        LLGL::PipelineLayoutDescriptor lightInitPipelineLayoutDesc;
        lightInitPipelineLayoutDesc.heapBindings = sge::BindingLayout(
            LLGL::StageFlags::ComputeStage,
            {
                sge::BindingLayoutItem::ConstantBuffer(3, "GlobalUniformBuffer"),
                sge::BindingLayoutItem::Buffer(4, "LightBuffer"),
                sge::BindingLayoutItem::TextureStorage(6, "LightTexture")
            }
        );

        LLGL::PipelineLayout* lightInitPipelineLayout = context->CreatePipelineLayout(lightInitPipelineLayoutDesc);

        const LLGL::ResourceViewDescriptor lightInitResourceViews[] = {
            m_renderer->GlobalUniformBuffer(), m_light_buffer, nullptr
        };

        LLGL::ResourceHeapDescriptor lightResourceHeapDesc;
        lightResourceHeapDesc.pipelineLayout = lightInitPipelineLayout;
        lightResourceHeapDesc.numResourceViews = ARRAY_LEN(lightInitResourceViews);

        m_light_init_resource_heap = context->CreateResourceHeap(lightResourceHeapDesc, lightInitResourceViews);

        LLGL::ComputePipelineDescriptor lightPipelineDesc;
        lightPipelineDesc.debugName = "WorldLightSetLightSourcesComputePipeline";
        lightPipelineDesc.pipelineLayout = lightInitPipelineLayout;
        lightPipelineDesc.computeShader = Assets::GetComputeShader(ComputeShaderAsset::LightSetLightSources);

        m_light_set_light_sources_pipeline = context->CreatePipelineState(lightPipelineDesc);
    }
    {
        LLGL::PipelineLayoutDescriptor lightBlurPipelineLayoutDesc;
        lightBlurPipelineLayoutDesc.heapBindings = sge::BindingLayout(
            LLGL::StageFlags::ComputeStage,
            {
                sge::BindingLayoutItem::ConstantBuffer(3, "GlobalUniformBuffer"),
                backend.IsOpenGL()
                    ? sge::BindingLayoutItem::TextureStorage(5, "TileTexture")
                    : sge::BindingLayoutItem::Texture(5, "TileTexture"),
                sge::BindingLayoutItem::TextureStorage(6, "LightTexture")
            }
        );
        lightBlurPipelineLayoutDesc.uniforms = {
            LLGL::UniformDescriptor("uniform_min", LLGL::UniformType::UInt2),
            LLGL::UniformDescriptor("uniform_max", LLGL::UniformType::UInt2),
        };

        LLGL::PipelineLayout* lightBlurPipelineLayout = context->CreatePipelineLayout(lightBlurPipelineLayoutDesc);

        const LLGL::ResourceViewDescriptor lightBlurResourceViews[] = {
            m_renderer->GlobalUniformBuffer(), nullptr, nullptr
        };

        LLGL::ResourceHeapDescriptor lightBlurResourceHeapDesc;
        lightBlurResourceHeapDesc.pipelineLayout = lightBlurPipelineLayout;
        lightBlurResourceHeapDesc.numResourceViews = ARRAY_LEN(lightBlurResourceViews);

        m_light_blur_resource_heap = context->CreateResourceHeap(lightBlurResourceHeapDesc, lightBlurResourceViews);

        LLGL::ComputePipelineDescriptor lightPipelineDesc;
        lightPipelineDesc.pipelineLayout = lightBlurPipelineLayout;

        lightPipelineDesc.debugName = "WorldLightVerticalComputePipeline";
        lightPipelineDesc.computeShader = Assets::GetComputeShader(ComputeShaderAsset::LightVertical);
        m_light_vertical_pipeline = context->CreatePipelineState(lightPipelineDesc);

        lightPipelineDesc.debugName = "WorldLightHorizontalComputePipeline";
        lightPipelineDesc.computeShader = Assets::GetComputeShader(ComputeShaderAsset::LightHorizontal);
        m_light_horizontal_pipeline = context->CreatePipelineState(lightPipelineDesc);
    }
}

void AcceleratedDynamicLighting::init_textures(const WorldData& world) {
    ZoneScopedN("WorldRenderer::init_textures");

    using Constants::SUBDIVISION;

    auto& context = m_renderer->Context();
    const sge::RenderBackend backend = m_renderer->Backend();

    SGE_RESOURCE_RELEASE(m_tile_texture);

    LLGL::TextureDescriptor tile_texture_desc;
    tile_texture_desc.type      = LLGL::TextureType::Texture2D;
    tile_texture_desc.format    = LLGL::Format::R8UInt;
    tile_texture_desc.extent    = LLGL::Extent3D(world.area.width(), world.area.height(), 1);
    tile_texture_desc.miscFlags = 0;
    tile_texture_desc.bindFlags = backend.IsOpenGL() ? LLGL::BindFlags::Storage : LLGL::BindFlags::Sampled;
    tile_texture_desc.mipLevels = 1;

    {
        const size_t size = world.area.width() * world.area.height();
        uint8_t* data = new uint8_t[size];
        for (int y = 0; y < world.area.height(); ++y) {
            for (int x = 0; x < world.area.width(); ++x) {
                uint8_t tile = world.tile_exists(TilePos(x, y)) ? 1 : 0;
                data[y * world.area.width() + x] = tile;
            }
        }

        LLGL::ImageView image_view;
        image_view.format   = LLGL::ImageFormat::R;
        image_view.dataType = LLGL::DataType::UInt8;
        image_view.data     = data;
        image_view.dataSize = size;

        m_tile_texture = context->CreateTexture(tile_texture_desc, &image_view);
    }

    context->WriteResourceHeap(*m_light_init_resource_heap, 2, {m_light_texture});
    context->WriteResourceHeap(*m_light_blur_resource_heap, 1, {m_tile_texture, m_light_texture});
}

void AcceleratedDynamicLighting::update(WorldData& world) {
    update_tile_texture(world);
}

void AcceleratedDynamicLighting::compute_light(const sge::Camera& camera, const World& world) {
    ZoneScopedN("WorldRenderer::compute_light");

    if (world.light_count() == 0) return;

    auto* const commands = m_renderer->CommandBuffer();

    const size_t size = world.light_count() * sizeof(Light);
    commands->UpdateBuffer(*m_light_buffer, 0, world.lights(), size);

    const glm::ivec2 proj_area_min = glm::ivec2((camera.position() + camera.get_projection_area().min) / Constants::TILE_SIZE) - static_cast<int>(m_workgroup_size);
    const glm::ivec2 proj_area_max = glm::ivec2((camera.position() + camera.get_projection_area().max) / Constants::TILE_SIZE) + static_cast<int>(m_workgroup_size);

    const sge::URect blur_area = sge::URect(
        glm::uvec2(glm::max(proj_area_min * Constants::SUBDIVISION, glm::ivec2(0))),
        glm::uvec2(glm::max(proj_area_max * Constants::SUBDIVISION, glm::ivec2(0)))
    );

    const uint32_t grid_w = blur_area.width() / m_workgroup_size;
    const uint32_t grid_h = blur_area.height() / m_workgroup_size;

    if (grid_w * grid_h == 0) return;

    commands->PushDebugGroup("CS Light SetLightSources");
    {
        commands->SetPipelineState(*m_light_set_light_sources_pipeline);
        commands->SetResourceHeap(*m_light_init_resource_heap);
        commands->Dispatch(world.light_count(), 1, 1);
    }
    commands->PopDebugGroup();

    const auto blur_horizontal = [&blur_area, commands, grid_w, this] {
        commands->PushDebugGroup("CS Light BlurHorizontal");
        {
            commands->SetPipelineState(*m_light_horizontal_pipeline);
            commands->SetResourceHeap(*m_light_blur_resource_heap);
            commands->SetUniforms(0, &blur_area.min, sizeof(blur_area.min));
            commands->SetUniforms(1, &blur_area.max, sizeof(blur_area.max));
            m_dispatch_func(commands, grid_w);
        }
        commands->PopDebugGroup();
    };

    const auto blur_vertical = [&blur_area, commands, grid_h, this] {
        commands->PushDebugGroup("CS Light BlurVertical");
        {
            commands->SetPipelineState(*m_light_vertical_pipeline);
            commands->SetResourceHeap(*m_light_blur_resource_heap);
            commands->SetUniforms(0, &blur_area.min, sizeof(blur_area.min));
            commands->SetUniforms(1, &blur_area.max, sizeof(blur_area.max));
            m_dispatch_func(commands, grid_h);
        }
        commands->PopDebugGroup();
    };

    for (int i = 0; i < 2; ++i) {
        blur_horizontal();  
        commands->ResourceBarrier(0, nullptr, 1, &m_light_texture);

        blur_vertical();
        commands->ResourceBarrier(0, nullptr, 1, &m_light_texture);
    }
    blur_horizontal();
}

void AcceleratedDynamicLighting::update_tile_texture(WorldData& world) {
    LLGL::ImageView image_view;
    image_view.format   = LLGL::ImageFormat::R;
    image_view.dataType = LLGL::DataType::UInt8;
    image_view.dataSize = 1;

    while (!world.changed_tiles.empty()) {
        auto [pos, value] = world.changed_tiles.back();
        world.changed_tiles.pop_back();

        image_view.data     = &value;
        m_renderer->Context()->WriteTexture(*m_tile_texture, LLGL::TextureRegion(LLGL::Offset3D(pos.x, pos.y, 0), LLGL::Extent3D(1, 1, 1)), image_view);
    }
}

void AcceleratedDynamicLighting::destroy() {
    const auto& context = m_renderer->Context();

    SGE_RESOURCE_RELEASE(m_light_set_light_sources_pipeline);
    SGE_RESOURCE_RELEASE(m_light_vertical_pipeline);
    SGE_RESOURCE_RELEASE(m_light_horizontal_pipeline);
    SGE_RESOURCE_RELEASE(m_light_init_resource_heap);
    SGE_RESOURCE_RELEASE(m_light_blur_resource_heap);
    SGE_RESOURCE_RELEASE(m_tile_texture);
}