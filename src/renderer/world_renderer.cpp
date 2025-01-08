#include "world_renderer.hpp"

#include <LLGL/Utils/Utility.h>
#include <LLGL/PipelineStateFlags.h>
#include <LLGL/ResourceHeapFlags.h>
#include <LLGL/TextureFlags.h>
#include <LLGL/Types.h>

#include <cstddef>
#include <glm/gtc/type_ptr.hpp>

#include <tracy/Tracy.hpp>

#include "../assets.hpp"
#include "../world/chunk.hpp"
#include "../utils.hpp"

#include "renderer.hpp"
#include "macros.hpp"

struct __attribute__((aligned(16))) DepthUniformData {
    float tile_depth;
    float wall_depth;
};

void WorldRenderer::init() {
    ZoneScopedN("WorldRenderer::init");

    const auto& context = Renderer::Context();

    auto depth_uniform = DepthUniformData {
        .tile_depth = 2,
        .wall_depth = 1
    };

    m_depth_buffer = context->CreateBuffer(LLGL::ConstantBufferDesc(sizeof(DepthUniformData)), &depth_uniform);

    LLGL::PipelineLayoutDescriptor pipelineLayoutDesc;
    pipelineLayoutDesc.heapBindings = {
        LLGL::BindingDescriptor(
            "GlobalUniformBuffer",
            LLGL::ResourceType::Buffer,
            LLGL::BindFlags::ConstantBuffer,
            LLGL::StageFlags::VertexStage,
            LLGL::BindingSlot(2)
        ),
        LLGL::BindingDescriptor(
            "DepthBuffer",
            LLGL::ResourceType::Buffer,
            LLGL::BindFlags::ConstantBuffer,
            LLGL::StageFlags::VertexStage,
            LLGL::BindingSlot(3)
        ),
    };
    pipelineLayoutDesc.bindings = {
        LLGL::BindingDescriptor("u_texture_array", LLGL::ResourceType::Texture, LLGL::BindFlags::Sampled, LLGL::StageFlags::FragmentStage, LLGL::BindingSlot(4)),
    };
    pipelineLayoutDesc.staticSamplers = {
        LLGL::StaticSamplerDescriptor("u_sampler", LLGL::StageFlags::FragmentStage, 5, Assets::GetSampler(TextureSampler::Nearest).descriptor()),
    };

    LLGL::PipelineLayout* pipelineLayout = context->CreatePipelineLayout(pipelineLayoutDesc);

    const LLGL::ResourceViewDescriptor resource_views[] = {
        Renderer::GlobalUniformBuffer(), m_depth_buffer
    };

    m_resource_heap = context->CreateResourceHeap(pipelineLayout, resource_views);

    const ShaderPipeline& tilemap_shader = Assets::GetShader(ShaderAsset::TilemapShader);

    LLGL::GraphicsPipelineDescriptor pipelineDesc;
    pipelineDesc.debugName = "World Pipeline";
    pipelineDesc.vertexShader = tilemap_shader.vs;
    pipelineDesc.fragmentShader = tilemap_shader.ps;
    pipelineDesc.geometryShader = tilemap_shader.gs;
    pipelineDesc.pipelineLayout = pipelineLayout;
    pipelineDesc.indexFormat = LLGL::Format::R16UInt;
    pipelineDesc.primitiveTopology = LLGL::PrimitiveTopology::TriangleStrip;
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
                .srcAlpha = LLGL::BlendOp::SrcAlpha,
                .dstAlpha = LLGL::BlendOp::InvSrcAlpha
            }
        }
    };

    m_pipeline = context->CreatePipelineState(pipelineDesc);

    // =======================================================

    LLGL::PipelineLayoutDescriptor lightPipelineLayoutDesc;
    lightPipelineLayoutDesc.heapBindings = {
        LLGL::BindingDescriptor(
            "GlobalUniformBuffer",
            LLGL::ResourceType::Buffer,
            LLGL::BindFlags::ConstantBuffer,
            LLGL::StageFlags::ComputeStage,
            LLGL::BindingSlot(3)
        ),
        LLGL::BindingDescriptor(
            "LightBuffer",
            LLGL::ResourceType::Buffer,
            LLGL::BindFlags::Sampled,
            LLGL::StageFlags::ComputeStage,
            LLGL::BindingSlot(4)
        ),
        LLGL::BindingDescriptor(
            "TileTexture",
            LLGL::ResourceType::Texture,
            LLGL::BindFlags::Sampled,
            LLGL::StageFlags::ComputeStage,
            LLGL::BindingSlot(5)
        ),
        LLGL::BindingDescriptor(
            "LightTexture",
            LLGL::ResourceType::Texture,
            LLGL::BindFlags::Storage,
            LLGL::StageFlags::ComputeStage,
            LLGL::BindingSlot(6)
        ),
    };
    lightPipelineLayoutDesc.uniforms = {
        LLGL::UniformDescriptor("uniform_min", LLGL::UniformType::UInt2),
        LLGL::UniformDescriptor("uniform_max", LLGL::UniformType::UInt2),
    };

    LLGL::PipelineLayout* lightPipelineLayout = context->CreatePipelineLayout(lightPipelineLayoutDesc);

    LLGL::BufferDescriptor light_buffer;
    light_buffer.bindFlags = LLGL::BindFlags::Sampled;
    light_buffer.stride = sizeof(Light);
    light_buffer.size = sizeof(Light) * WORLD_MAX_LIGHT_COUNT;
    m_light_buffer = context->CreateBuffer(light_buffer);

    const LLGL::ResourceViewDescriptor lightResourceViews[] = {
        Renderer::GlobalUniformBuffer(), m_light_buffer, nullptr, nullptr
    };

    LLGL::ResourceHeapDescriptor lightResourceHeapDesc;
    lightResourceHeapDesc.pipelineLayout = lightPipelineLayout;
    lightResourceHeapDesc.numResourceViews = ARRAY_LEN(lightResourceViews);

    m_light_resource_heap = context->CreateResourceHeap(lightResourceHeapDesc, lightResourceViews);

    {
        LLGL::ComputePipelineDescriptor lightPipelineDesc;
        lightPipelineDesc.debugName = "WorldLightSetLightSourcesComputePipeline";
        lightPipelineDesc.pipelineLayout = lightPipelineLayout;
        lightPipelineDesc.computeShader = Assets::GetComputeShader(ComputeShaderAsset::LightSetLightSources);

        m_light_set_light_sources_pipeline = context->CreatePipelineState(lightPipelineDesc);
    }

    {
        LLGL::ComputePipelineDescriptor lightPipelineDesc;
        lightPipelineDesc.debugName = "WorldLightVerticalComputePipeline";
        lightPipelineDesc.pipelineLayout = lightPipelineLayout;
        lightPipelineDesc.computeShader = Assets::GetComputeShader(ComputeShaderAsset::LightVertical);

        m_light_vertical_pipeline = context->CreatePipelineState(lightPipelineDesc);
    }
    {
        LLGL::ComputePipelineDescriptor lightPipelineDesc;
        lightPipelineDesc.debugName = "WorldLightHorizontalComputePipeline";
        lightPipelineDesc.pipelineLayout = lightPipelineLayout;
        lightPipelineDesc.computeShader = Assets::GetComputeShader(ComputeShaderAsset::LightHorizontal);

        m_light_horizontal_pipeline = context->CreatePipelineState(lightPipelineDesc);
    }
}

void WorldRenderer::init_textures(const WorldData& world) {
    ZoneScopedN("WorldRenderer::init_textures");

    using Constants::SUBDIVISION;

    auto& context = Renderer::Context();

    RESOURCE_RELEASE(m_lightmap_texture);
    RESOURCE_RELEASE(m_light_texture);
    RESOURCE_RELEASE(m_tile_texture);
    RESOURCE_RELEASE(m_light_texture_target);

    {
        LLGL::TextureDescriptor lightmap_texture_desc;
        lightmap_texture_desc.type      = LLGL::TextureType::Texture2D;
        lightmap_texture_desc.format    = LLGL::Format::R8UNorm;
        lightmap_texture_desc.extent    = LLGL::Extent3D(world.lightmap.width, world.lightmap.height, 1);
        lightmap_texture_desc.miscFlags = 0;
        lightmap_texture_desc.bindFlags = LLGL::BindFlags::Sampled;

        LLGL::ImageView image_view;
        image_view.format   = LLGL::ImageFormat::R;
        image_view.dataType = LLGL::DataType::UInt8;
        image_view.data     = world.lightmap.colors;
        image_view.dataSize = world.lightmap.width * world.lightmap.height * sizeof(Color);

        m_lightmap_texture = context->CreateTexture(lightmap_texture_desc, &image_view);
    }
    {
        LLGL::TextureDescriptor light_texture_desc;
        light_texture_desc.type      = LLGL::TextureType::Texture2D;
        light_texture_desc.format    = LLGL::Format::RGBA8UNorm;
        light_texture_desc.extent    = LLGL::Extent3D(world.lightmap.width, world.lightmap.height, 1);
        light_texture_desc.miscFlags = 0;
        light_texture_desc.bindFlags = LLGL::BindFlags::Storage | LLGL::BindFlags::Sampled | LLGL::BindFlags::ColorAttachment;

        const size_t size = world.lightmap.width * world.lightmap.height * 3;
        uint8_t* data = new uint8_t[size];
        memset(data, 0, size);

        LLGL::ImageView image_view;
        image_view.format   = LLGL::ImageFormat::RGB;
        image_view.dataType = LLGL::DataType::UInt8;
        image_view.data     = data;
        image_view.dataSize = size;

        m_light_texture = context->CreateTexture(light_texture_desc, &image_view);
    }

    LLGL::TextureDescriptor tile_texture_desc;
    tile_texture_desc.type      = LLGL::TextureType::Texture2D;
    tile_texture_desc.format    = LLGL::Format::R8UInt;
    tile_texture_desc.extent    = LLGL::Extent3D(world.area.width(), world.area.height(), 1);
    tile_texture_desc.miscFlags = 0;
    tile_texture_desc.bindFlags = LLGL::BindFlags::Sampled;

    {
        const size_t size = world.area.width() * world.area.height();
        uint8_t* data = new uint8_t[size];
        for (int y = 0; y < world.area.height(); ++y) {
            for (int x = 0; x < world.area.width(); ++x) {
                uint8_t tile = world.block_exists(TilePos(x, y)) ? 1 : 0;
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

    context->WriteResourceHeap(*m_light_resource_heap, 2, {m_tile_texture, m_light_texture});

    LLGL::RenderTargetDescriptor lightTextureRenderTarget;
    lightTextureRenderTarget.resolution = LLGL::Extent2D(world.lightmap.width, world.lightmap.height);
    lightTextureRenderTarget.colorAttachments[0].texture = m_light_texture;

    m_light_texture_target = context->CreateRenderTarget(lightTextureRenderTarget);
}

inline static void internal_update_lightmap_texture(const WorldData& world, const LightMapTaskResult& result, LLGL::Texture* lightmap_texture, LLGL::ImageView& image_view) {
    for (int y = 0; y < result.height; ++y) {
        memcpy(&world.lightmap.colors[(result.offset_y + y) * world.lightmap.width + result.offset_x], &result.data[y * result.width], result.width * sizeof(Color));
        memcpy(&world.lightmap.masks[(result.offset_y + y) * world.lightmap.width + result.offset_x], &result.mask[y * result.width], result.width * sizeof(LightMask));
    }

    image_view.data     = result.data;
    image_view.dataSize = result.width * result.height * sizeof(Color);
    Renderer::Context()->WriteTexture(*lightmap_texture, LLGL::TextureRegion(LLGL::Offset3D(result.offset_x, result.offset_y, 0), LLGL::Extent3D(result.width, result.height, 1)), image_view);
}

void WorldRenderer::update_lightmap_texture(WorldData& world) {
    ZoneScopedN("WorldRenderer::update_lightmap_texture");

    LLGL::ImageView image_view;
    image_view.format   = LLGL::ImageFormat::R;
    image_view.dataType = LLGL::DataType::UInt8;

    for (auto it = world.lightmap_tasks.cbegin(); it != world.lightmap_tasks.cend();) {
        const LightMapTask& task = *it;
        const LightMapTaskResult result = task.result->load();

        if (result.is_complete) {
            internal_update_lightmap_texture(world, result, m_lightmap_texture, image_view);

            delete[] result.data;
            delete[] result.mask;
            it = world.lightmap_tasks.erase(it);
            continue;
        }

        ++it;
    }

}

void WorldRenderer::update_tile_texture(WorldData& world) {
    LLGL::ImageView image_view;
    image_view.format   = LLGL::ImageFormat::R;
    image_view.dataType = LLGL::DataType::UInt8;
    image_view.dataSize = 1;

    while (!world.changed_tiles.empty()) {
        auto [pos, value] = world.changed_tiles.back();
        world.changed_tiles.pop_back();

        image_view.data     = &value;
        Renderer::Context()->WriteTexture(*m_tile_texture, LLGL::TextureRegion(LLGL::Offset3D(pos.x, pos.y, 0), LLGL::Extent3D(1, 1, 1)), image_view);
    }
}

void WorldRenderer::render(const ChunkManager& chunk_manager) {
    ZoneScopedN("WorldRenderer::render");

    auto* const commands = Renderer::CommandBuffer();

    commands->SetPipelineState(*m_pipeline);

    const Texture& walls_texture = Assets::GetTexture(TextureAsset::Walls);
    const Texture& tiles_texture = Assets::GetTexture(TextureAsset::Tiles);

    for (const glm::uvec2& pos : chunk_manager.visible_chunks()) {
        const RenderChunk& chunk = chunk_manager.render_chunks().find(pos)->second;

        if (!chunk.walls_empty()) {
            commands->SetVertexBufferArray(*chunk.wall_buffer_array);
            commands->SetResource(0, walls_texture);
            commands->SetResourceHeap(*m_resource_heap);

            commands->DrawInstanced(4, 0, chunk.walls_count);
        }

        if (!chunk.blocks_empty()) {
            commands->SetVertexBufferArray(*chunk.block_buffer_array);
            commands->SetResource(0, tiles_texture);
            commands->SetResourceHeap(*m_resource_heap);

            commands->DrawInstanced(4, 0, chunk.blocks_count);
        }
    }
}

void WorldRenderer::compute_light(const Camera& camera, const World& world) {
    ZoneScopedN("WorldRenderer::compute_light");

    if (world.light_count() == 0) return;

    auto* const commands = Renderer::CommandBuffer();

    const size_t size = world.light_count() * sizeof(Light);
    commands->UpdateBuffer(*m_light_buffer, 0, world.lights(), size);

    constexpr uint32_t WORKGROUP = 16;

    const glm::ivec2 proj_area_min = glm::ivec2((camera.position() + camera.get_projection_area().min) / Constants::TILE_SIZE) - static_cast<int>(WORKGROUP);
    const glm::ivec2 proj_area_max = glm::ivec2((camera.position() + camera.get_projection_area().max) / Constants::TILE_SIZE) + static_cast<int>(WORKGROUP);

    math::URect blur_area = math::URect(
        glm::uvec2(glm::max(proj_area_min * Constants::SUBDIVISION, glm::ivec2(0))),
        glm::uvec2(glm::max(proj_area_max * Constants::SUBDIVISION, glm::ivec2(0)))
    );

    const uint32_t grid_w = blur_area.width() / WORKGROUP;
    const uint32_t grid_h = blur_area.height() / WORKGROUP;

    if (grid_w * grid_h == 0) return;

    commands->PushDebugGroup("CS Light SetLightSources");
    {
        commands->SetPipelineState(*m_light_set_light_sources_pipeline);
        commands->SetResourceHeap(*m_light_resource_heap);
        commands->Dispatch(world.light_count(), 1, 1);
    }
    commands->PopDebugGroup();

    for (int i = 0; i < 2; ++i) {
        commands->PushDebugGroup("CS Light BlurHorizontal");
        {
            commands->SetPipelineState(*m_light_horizontal_pipeline);
            commands->SetResourceHeap(*m_light_resource_heap);
            commands->SetUniforms(0, &blur_area.min, sizeof(blur_area.min));
            commands->SetUniforms(1, &blur_area.max, sizeof(blur_area.max));
            commands->Dispatch(grid_w, 1, 1);
        }
        commands->PopDebugGroup();

        commands->PushDebugGroup("CS Light BlurVertical");
        {
            commands->SetPipelineState(*m_light_vertical_pipeline);
            commands->SetResourceHeap(*m_light_resource_heap);
            commands->SetUniforms(0, &blur_area.min, sizeof(blur_area.min));
            commands->SetUniforms(1, &blur_area.max, sizeof(blur_area.max));
            commands->Dispatch(grid_h, 1, 1);
        }
        commands->PopDebugGroup();
    }

    commands->PushDebugGroup("CS Light BlurHorizontal");
    {
        commands->SetPipelineState(*m_light_horizontal_pipeline);
        commands->SetResourceHeap(*m_light_resource_heap);
        commands->SetUniforms(0, &blur_area.min, sizeof(blur_area.min));
        commands->SetUniforms(1, &blur_area.max, sizeof(blur_area.max));
        commands->Dispatch(grid_w, 1, 1);
    }
    commands->PopDebugGroup();
}

void WorldRenderer::terminate() {
    RESOURCE_RELEASE(m_pipeline);
    RESOURCE_RELEASE(m_light_set_light_sources_pipeline);
    RESOURCE_RELEASE(m_light_vertical_pipeline);
    RESOURCE_RELEASE(m_light_horizontal_pipeline);
    RESOURCE_RELEASE(m_light_resource_heap);
    RESOURCE_RELEASE(m_resource_heap);
    RESOURCE_RELEASE(m_depth_buffer);
    RESOURCE_RELEASE(m_lightmap_texture);
    RESOURCE_RELEASE(m_light_texture);
    RESOURCE_RELEASE(m_light_texture_target);
    RESOURCE_RELEASE(m_tile_texture);
}