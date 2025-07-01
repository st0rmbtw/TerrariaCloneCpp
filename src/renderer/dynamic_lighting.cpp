#include <algorithm>
#include <tracy/Tracy.hpp>

#include <LLGL/Container/DynamicArray.h>
#include <LLGL/Tags.h>

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

DynamicLighting::DynamicLighting(const WorldData& world, LLGL::Texture* light_texture) : m_light_texture(light_texture) {
    m_dynamic_lightmap = LightMap(world.area.width(), world.area.height());
    m_renderer = &sge::Engine::Renderer();

    m_line = LLGL::DynamicArray<Color>(Constants::LIGHT_AIR_DECAY_STEPS);

    for (int y = 0; y < m_dynamic_lightmap.height; ++y) {
        for (int x = 0; x < m_dynamic_lightmap.width; ++x) {
            const TilePos color_pos = TilePos(x, y);
            const TilePos tile_pos = color_pos / Constants::SUBDIVISION;

            m_dynamic_lightmap.set_mask(color_pos, world.tile_exists(tile_pos));
        }
    }
}

SGE_FORCE_INLINE static void blur_line(LightMap& lightmap, int start, int end, int stride, glm::vec3& prev_light, float& prev_decay, glm::vec3& prev_light2, float& prev_decay2) {
    using Constants::LIGHT_EPSILON;

    int length = end - start;
    for (int index = 0; index < length; index += stride) {
        blur(lightmap, start + index, prev_light, prev_decay);
        blur(lightmap, end - index, prev_light2, prev_decay2);
    }
}

static constexpr unsigned g_maxThreadCountStaticArray = 64;

static void DoConcurrentRangeInWorkerContainer(
    const std::function<void(std::size_t begin, std::size_t end)>&  task,
    std::size_t                                                     count,
    std::size_t                                                     workerCount,
    std::thread*                                                    workers)
{
    /* Distribute work to threads */
    const std::size_t workSize          = count / workerCount;
    const std::size_t workSizeRemain    = count % workerCount;

    std::size_t offset = 0;

    for (size_t i = 0; i < workerCount; ++i)
    {
        workers[i] = std::thread(task, offset, offset + workSize);
        offset += workSize;
    }

    /* Execute task of remaining work on main thread */
    if (workSizeRemain > 0)
        task(offset, offset + workSizeRemain);

    /* Join worker threads */
    for (size_t i = 0; i < workerCount; ++i) workers[i].join();
}

static unsigned Log2Uint(unsigned n)
{
    unsigned nLog2 = 0;
    while (n >>= 1)
        ++nLog2;
    return nLog2;
}

static unsigned ClampThreadCount(unsigned threadCount, std::size_t workSize, unsigned threadMinWorkSize)
{
    if (workSize > threadMinWorkSize)
    {
        if (threadCount == LLGL_MAX_THREAD_COUNT)
        {
            /* Compute number of threads automatically logarithmically to the workload */
            threadCount = Log2Uint(static_cast<unsigned>(workSize / threadMinWorkSize));

            /*
            Clamp to maximum number of threads support by the CPU.
            If this value is undefined or not comutable, the return value of the STL function is 0.
            */
            const unsigned maxThreadCount = std::thread::hardware_concurrency();
            if (maxThreadCount > 0)
                threadCount = std::min(threadCount, maxThreadCount);
        }

        /* Clamp final number of threads by the minimum workload per thread */
        return std::min(threadCount, static_cast<unsigned>(workSize / threadMinWorkSize));
    }
    return 0;
}

LLGL_EXPORT void DoConcurrentRange(
    const std::function<void(std::size_t begin, std::size_t end)>&  task,
    std::size_t                                                     count,
    unsigned                                                        threadCount,
    unsigned                                                        threadMinWorkSize)
{
    threadCount = ClampThreadCount(threadCount, count, threadMinWorkSize);

    if (threadCount <= 1)
    {
        /* Run single-threaded */
        task(0, count);
    }
    else if (threadCount <= g_maxThreadCountStaticArray)
    {
        /* Launch worker threads in static array */
        std::thread workers[g_maxThreadCountStaticArray];
        DoConcurrentRangeInWorkerContainer(task, count, threadCount, workers);
    }
    else if (threadCount > 1)
    {
        /* Launch worker threads in dynamic array */
        std::vector<std::thread> workers(threadCount);
        DoConcurrentRangeInWorkerContainer(task, count, threadCount, workers.data());
    }
}

LLGL_EXPORT void DoConcurrent(
    const std::function<void(std::size_t index)>&   task,
    std::size_t                                     count,
    unsigned                                        threadCount = LLGL_MAX_THREAD_COUNT,
    unsigned                                        threadMinWorkSize = 64)
{
    DoConcurrentRange(
        [&task](std::size_t begin, std::size_t end)
        {
            for (size_t i = begin; i < end; ++i)
                task(i);
        },
        count,
        threadCount,
        threadMinWorkSize
    );
}

SGE_FORCE_INLINE static void blur_horizontal(LightMap& lightmap, const sge::IRect& area) {
    #pragma omp parallel for
    for (int y = area.min.y; y < area.max.y; ++y) {
        glm::vec3 prev_light = lightmap.get_color({area.min.x, y});
        float prev_decay = Constants::LightDecay(lightmap.get_mask({(area.min.x) - 1, y}));

        glm::vec3 prev_light2 = lightmap.get_color({area.max.x - 1, y});
        float prev_decay2 = Constants::LightDecay(lightmap.get_mask({(area.max.x), y}));

        blur_line(lightmap, y * lightmap.width + area.min.x, y * lightmap.width + (area.max.x - 1), 1, prev_light, prev_decay, prev_light2, prev_decay2);
    }
}

SGE_FORCE_INLINE static void blur_vertical(LightMap& lightmap, const sge::IRect& area) {
    #pragma omp parallel for
    for (int x = area.min.x; x < area.max.x; ++x) {
        glm::vec3 prev_light = lightmap.get_color({x, area.min.y});
        float prev_decay = Constants::LightDecay(lightmap.get_mask({x, (area.min.y) - 1}));

        glm::vec3 prev_light2 = lightmap.get_color({x, area.max.y - 1});
        float prev_decay2 = Constants::LightDecay(lightmap.get_mask({x, (area.max.y)}));

        blur_line(lightmap, area.min.y * lightmap.width + x, (area.max.y - 1) * lightmap.width + x, lightmap.width, prev_light, prev_decay, prev_light2, prev_decay2);
    }
}

static uint32_t count_steps(const LightMap& lightmap, LLGL::DynamicArray<Color>& line, glm::vec3& prev_light, float& prev_decay, int start_index, int stride) {
    using Constants::LIGHT_EPSILON;

    int index = start_index;
    uint32_t i = 0;
    while (i < Constants::LIGHT_AIR_DECAY_STEPS && index >= 0) {
        glm::vec3 this_light = line[i].as_vec3();

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

        // The light has completely decayed - nothing to blur
        if (this_light.r < LIGHT_EPSILON && this_light.g < LIGHT_EPSILON && this_light.b < LIGHT_EPSILON) break;

        prev_light = prev_light * prev_decay;
        prev_decay = Constants::LightDecay(lightmap.get_mask(index));
        index += stride;
        i++;
    }

    return i;
}

static sge::IRect calculate_light_area(const LightMap& lightmap, LLGL::DynamicArray<Color>& line, glm::ivec2 pos, const glm::vec3& color) {
    glm::vec3 prev_light = glm::vec3(0.0f);
    float prev_decay = 0.0f;
    int length = 0;

    line[0] = Color(color);

    prev_light = glm::vec3(0.0f);

    sge::IRect area = sge::IRect();

    // Horizontal
    prev_decay = Constants::LightDecay(lightmap.get_mask({pos.x + 1, pos.y}));
    length = count_steps(lightmap, line, prev_light, prev_decay, (pos.y) * lightmap.width + pos.x, -1);
    area.min.x = glm::min(area.min.x, -length);

    prev_decay = Constants::LightDecay(lightmap.get_mask({pos.x - 1, pos.y}));
    length = count_steps(lightmap, line, prev_light, prev_decay, (pos.y) * lightmap.width + pos.x, 1);
    area.max.x = glm::max(area.max.x, length);

    prev_light = glm::vec3(0.0f);

    // Vertical
    prev_decay = Constants::LightDecay(lightmap.get_mask({pos.x, pos.y + 1}));
    length = count_steps(lightmap, line, prev_light, prev_decay, (pos.y) * lightmap.width + pos.x, -lightmap.width);
    area.min.y = glm::min(area.min.y, -length);

    prev_decay = Constants::LightDecay(lightmap.get_mask({pos.x, pos.y - 1}));
    length = count_steps(lightmap, line, prev_light, prev_decay, (pos.y) * lightmap.width + pos.x, lightmap.width);
    area.max.y = glm::max(area.max.y, length);

    return area;
}

void DynamicLighting::update(World& world) {
    const uint32_t light_count = world.light_count();
    if (light_count == 0) return;

    m_areas.clear();
    m_indices.clear();

    sge::IRect process_area = sge::IRect::uninitialized();
    bool initialized = false;

    LightMap& lightmap = m_dynamic_lightmap;

    size_t i = 0;
    for (i = 0; i < light_count; ++i) {
        const Light& light = world.lights()[i];

        glm::ivec2 light_pos = glm::max(glm::ivec2(light.pos), glm::ivec2(0));

        const sge::IRect area = calculate_light_area(lightmap, m_line, light_pos, light.color);
        const sge::IRect a = (area + light.pos).clamp(sge::IRect({0, 0}, {lightmap.width, lightmap.height}));

        if (!initialized) {
            process_area = a;
            initialized = true;
        }

        if (process_area.intersects(a)) {
            process_area = process_area.merge(a);
            m_indices.emplace_back(i, m_areas.size());
        } else {
            m_areas.push_back(process_area);

            m_indices.emplace_back(i, m_areas.size());
            m_areas.push_back(a);

            process_area = sge::IRect::uninitialized();
            initialized = false;
        }
    }

    if (initialized) {
        m_areas.push_back(process_area);
    }
}

void DynamicLighting::compute_light(const sge::Camera& camera, const World& world) {
    const uint32_t light_count = world.light_count();
    if (light_count == 0) return;

    LightMap& lightmap = m_dynamic_lightmap;

    const glm::ivec2 proj_area_min = glm::ivec2((camera.position() + camera.get_projection_area().min) / Constants::TILE_SIZE) - 8;
    const glm::ivec2 proj_area_max = glm::ivec2((camera.position() + camera.get_projection_area().max) / Constants::TILE_SIZE) + 8;

    const sge::URect screen_blur_area = sge::URect(
        glm::uvec2(glm::max(proj_area_min * Constants::SUBDIVISION, glm::ivec2(0))),
        glm::uvec2(glm::max(proj_area_max * Constants::SUBDIVISION, glm::ivec2(0)))
    );

    for (sge::IRect& area : m_areas) {
        // Clamp to screen resolution
        area = area.clamp(screen_blur_area);

        // Clear
        for (int y = area.min.y; y < area.max.y; ++y) {
            memset(&lightmap.colors[y * lightmap.width + area.min.x], 0, area.width() * sizeof(Color));
        }
    }

    for (size_t i = 0; i < world.light_count(); ++i) {
        const Light& light = world.lights()[i];

        // Init
        for (uint32_t y = 0; y < light.size.y; ++y) {
            for (uint32_t x = 0; x < light.size.x; ++x) {
                lightmap.set_color(light.pos + TilePos(x, y), light.color);
            }
        }
    }

    // Blur
    DoConcurrent([&lightmap, this](size_t i) {
        const sge::IRect& area = m_areas[i];

        for (size_t i = 0; i < 2; ++i) {
            blur_horizontal(lightmap, area);

            blur_vertical(lightmap, area);
        }

        blur_horizontal(lightmap, area);
    }, m_areas.size(), m_areas.size(), 1);

    const auto& context = m_renderer->Context();

    for (const sge::IRect& area : m_areas) {
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
    is_metal = backend.IsMetal();

    if (backend.IsMetal())
        m_workgroup_size = 1;

    init_pipeline();
    init_textures(world);
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

void AcceleratedDynamicLighting::init_pipeline() {
    using Constants::WORLD_MAX_LIGHT_COUNT;

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
        LLGL::DynamicArray<uint8_t> pixels(world.area.width() * world.area.height(), LLGL::UninitializeTag{});

        for (int y = 0; y < world.area.height(); ++y) {
            for (int x = 0; x < world.area.width(); ++x) {
                uint8_t tile = world.tile_exists(TilePos(x, y)) ? 1 : 0;
                pixels[y * world.area.width() + x] = tile;
            }
        }

        LLGL::ImageView image_view;
        image_view.format   = LLGL::ImageFormat::R;
        image_view.dataType = LLGL::DataType::UInt8;
        image_view.data     = pixels.data();
        image_view.dataSize = pixels.size();

        m_tile_texture = context->CreateTexture(tile_texture_desc, &image_view);
    }

    context->WriteResourceHeap(*m_light_init_resource_heap, 2, {m_light_texture});
    context->WriteResourceHeap(*m_light_blur_resource_heap, 1, {m_tile_texture, m_light_texture});
}

void AcceleratedDynamicLighting::compute_light(const sge::Camera& camera, const World& world) {
    ZoneScopedN("WorldRenderer::compute_light");

    if (world.light_count() == 0) return;

    auto* const commands = m_renderer->CommandBuffer();

    const size_t size = world.light_count() * sizeof(Light);
    commands->UpdateBuffer(*m_light_buffer, 0, world.lights(), size);

    const glm::ivec2 proj_area_min = glm::ivec2((camera.position() + camera.get_projection_area().min) / Constants::TILE_SIZE) - 16;
    const glm::ivec2 proj_area_max = glm::ivec2((camera.position() + camera.get_projection_area().max) / Constants::TILE_SIZE) + 16;

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

            if (is_metal) {
                blur_dispatch_metal(commands, grid_w);
            } else {
                blur_dispatch(commands, grid_w);
            }
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

            if (is_metal) {
                blur_dispatch_metal(commands, grid_h);
            } else {
                blur_dispatch(commands, grid_h);
            }
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