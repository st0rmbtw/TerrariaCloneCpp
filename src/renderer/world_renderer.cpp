#include "world_renderer.hpp"

#include <LLGL/Utils/Utility.h>
#include <glm/gtc/type_ptr.hpp>

#include "../assets.hpp"
#include "../world/chunk.hpp"

#include "LLGL/PipelineStateFlags.h"
#include "LLGL/TextureFlags.h"
#include "LLGL/Types.h"
#include "renderer.hpp"

struct __attribute__((aligned(16))) DepthUniformData {
    float tile_depth;
    float wall_depth;
};

void WorldRenderer::init() {
    const auto& context = Renderer::Context();

    auto depth_uniform = DepthUniformData {
        .tile_depth = 2,
        .wall_depth = 1
    };

    m_depth_buffer = context->CreateBuffer(LLGL::ConstantBufferDesc(sizeof(DepthUniformData)), &depth_uniform);

    const auto* render_pass = Renderer::DefaultRenderPass();
    const RenderBackend backend = Renderer::Backend();

    LLGL::PipelineLayoutDescriptor pipelineLayoutDesc;
    pipelineLayoutDesc.bindings = {
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
        LLGL::BindingDescriptor("u_texture_array", LLGL::ResourceType::Texture, LLGL::BindFlags::Sampled, LLGL::StageFlags::FragmentStage, LLGL::BindingSlot(4)),
        LLGL::BindingDescriptor("u_sampler", LLGL::ResourceType::Sampler, 0, LLGL::StageFlags::FragmentStage, backend.IsOpenGL() ? 4 : 5),
    };

    LLGL::PipelineLayout* pipelineLayout = context->CreatePipelineLayout(pipelineLayoutDesc);

    const ShaderPipeline& tilemap_shader = Assets::GetShader(ShaderAsset::TilemapShader);

    LLGL::GraphicsPipelineDescriptor pipelineDesc;
    pipelineDesc.debugName = "World Pipeline";
    pipelineDesc.vertexShader = tilemap_shader.vs;
    pipelineDesc.fragmentShader = tilemap_shader.ps;
    pipelineDesc.geometryShader = tilemap_shader.gs;
    pipelineDesc.pipelineLayout = pipelineLayout;
    pipelineDesc.indexFormat = LLGL::Format::Undefined;
    pipelineDesc.primitiveTopology = LLGL::PrimitiveTopology::TriangleStrip;
    pipelineDesc.renderPass = render_pass;
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
    for (int y = 0; y < result.height; ++y) {
        LLGL::ImageView image_view;
        image_view.format   = LLGL::ImageFormat::RGBA;
        image_view.dataType = LLGL::DataType::UInt8;
        image_view.data     = &result.data[y * result.width];
        image_view.dataSize = result.width * 1 * 4;

        memcpy(&world.lightmap.colors[(result.offset_y + y) * world.lightmap.width + result.offset_x], &result.data[y * result.width], result.width * sizeof(Color));
        memcpy(&world.lightmap.masks[(result.offset_y + y) * world.lightmap.width + result.offset_x], &result.mask[y * result.width], result.width * sizeof(LightMask));

        Renderer::Context()->WriteTexture(*m_lightmap_texture, LLGL::TextureRegion(LLGL::Offset3D(result.offset_x, result.offset_y + y, 0), LLGL::Extent3D(result.width, 1, 1)), image_view);
    }
}

void WorldRenderer::render(const ChunkManager& chunk_manager) {
    auto* const commands = Renderer::CommandBuffer();

    commands->SetPipelineState(*m_pipeline);

    const Texture& walls_texture = Assets::GetTexture(TextureAsset::Walls);
    const Texture& tiles_texture = Assets::GetTexture(TextureAsset::Tiles);

    for (const glm::uvec2& pos : chunk_manager.visible_chunks()) {
        const RenderChunk& chunk = chunk_manager.render_chunks().find(pos)->second;

        if (!chunk.walls_empty()) {
            commands->SetVertexBufferArray(*chunk.wall_buffer_array);
            commands->SetResource(0, *Renderer::GlobalUniformBuffer());
            commands->SetResource(1, *m_depth_buffer);
            commands->SetResource(2, *walls_texture.texture);
            commands->SetResource(3, Assets::GetSampler(walls_texture.sampler));

            commands->DrawInstanced(4, 0, chunk.walls_count);
        }

        if (!chunk.blocks_empty()) {
            commands->SetVertexBufferArray(*chunk.block_buffer_array);
            commands->SetResource(0, *Renderer::GlobalUniformBuffer());
            commands->SetResource(1, *m_depth_buffer);
            commands->SetResource(2, *tiles_texture.texture);
            commands->SetResource(3, Assets::GetSampler(tiles_texture.sampler));

            commands->DrawInstanced(4, 0, chunk.blocks_count);
        }
    }
}

void WorldRenderer::terminate() {
    if (m_pipeline) Renderer::Context()->Release(*m_pipeline);
    if (m_depth_buffer) Renderer::Context()->Release(*m_depth_buffer);
    if (m_lightmap_texture) Renderer::Context()->Release(*m_lightmap_texture);
}