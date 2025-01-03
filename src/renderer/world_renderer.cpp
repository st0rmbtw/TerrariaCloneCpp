#include "world_renderer.hpp"

#include <LLGL/Utils/Utility.h>
#include <LLGL/PipelineStateFlags.h>
#include <LLGL/ResourceHeapFlags.h>
#include <LLGL/TextureFlags.h>
#include <LLGL/Types.h>

#include <glm/gtc/type_ptr.hpp>

#include <tracy/Tracy.hpp>

#include "../assets.hpp"
#include "../world/chunk.hpp"

#include "renderer.hpp"

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
}

void WorldRenderer::init_lightmap_texture(const WorldData& world) {
    ZoneScopedN("WorldRenderer::init_lightmap_texture");

    using Constants::SUBDIVISION;

    auto& context = Renderer::Context();

    if (m_lightmap_texture) context->Release(*m_lightmap_texture);

    LLGL::TextureDescriptor lightmap_texture_desc;
    lightmap_texture_desc.type      = LLGL::TextureType::Texture2D;
    lightmap_texture_desc.format    = LLGL::Format::RGBA8UNorm;
    lightmap_texture_desc.extent    = LLGL::Extent3D(world.lightmap.width, world.lightmap.height, 1);
    lightmap_texture_desc.miscFlags = 0;
    lightmap_texture_desc.bindFlags = LLGL::BindFlags::Sampled;

    LLGL::ImageView image_view;
    image_view.format   = LLGL::ImageFormat::RGBA;
    image_view.dataType = LLGL::DataType::UInt8;
    image_view.data     = world.lightmap.colors;
    image_view.dataSize = world.lightmap.width * world.lightmap.height * 4;

    m_lightmap_texture = context->CreateTexture(lightmap_texture_desc, &image_view);
}

void WorldRenderer::update_lightmap_texture(WorldData& world, LightMapTaskResult result) {
    ZoneScopedN("WorldRenderer::update_lightmap_texture");

    for (int y = 0; y < result.height; ++y) {
        memcpy(&world.lightmap.colors[(result.offset_y + y) * world.lightmap.width + result.offset_x], &result.data[y * result.width], result.width * sizeof(Color));
        memcpy(&world.lightmap.masks[(result.offset_y + y) * world.lightmap.width + result.offset_x], &result.mask[y * result.width], result.width * sizeof(LightMask));
    }

    LLGL::ImageView image_view;
    image_view.format   = LLGL::ImageFormat::RGBA;
    image_view.dataType = LLGL::DataType::UInt8;
    image_view.data     = result.data;
    image_view.dataSize = result.width * result.height * 4;

    Renderer::Context()->WriteTexture(*m_lightmap_texture, LLGL::TextureRegion(LLGL::Offset3D(result.offset_x, result.offset_y, 0), LLGL::Extent3D(result.width, result.height, 1)), image_view);
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

void WorldRenderer::terminate() {
    if (m_pipeline) Renderer::Context()->Release(*m_pipeline);
    if (m_depth_buffer) Renderer::Context()->Release(*m_depth_buffer);
    if (m_lightmap_texture) Renderer::Context()->Release(*m_lightmap_texture);
}