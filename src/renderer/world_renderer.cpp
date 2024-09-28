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
    lightmap_texture_desc.extent    = LLGL::Extent3D(world.area.width() * SUBDIVISION, world.area.height() * SUBDIVISION, 1);
    lightmap_texture_desc.miscFlags = 0;
    lightmap_texture_desc.bindFlags = LLGL::BindFlags::Sampled;

    LLGL::ImageView image_view;
    image_view.format   = LLGL::ImageFormat::RGBA;
    image_view.dataType = LLGL::DataType::UInt8;
    image_view.data     = world.lightmap;
    image_view.dataSize = world.area.width() * SUBDIVISION * world.area.height() * SUBDIVISION * 4;

    m_lightmap_texture = context->CreateTexture(lightmap_texture_desc, &image_view);
}

void WorldRenderer::render(const World& world) {
    auto* const commands = Renderer::CommandBuffer();

    using Constants::SUBDIVISION;

    const int width = world.area().width() * SUBDIVISION;
    const int height = world.area().height() * SUBDIVISION;

    LLGL::ImageView image_view;
    image_view.format   = LLGL::ImageFormat::RGBA;
    image_view.dataType = LLGL::DataType::UInt8;
    image_view.data     = world.data().lightmap;
    image_view.dataSize = width * height * 4;

    Renderer::Context()->WriteTexture(*m_lightmap_texture, LLGL::TextureRegion(LLGL::Offset3D(), LLGL::Extent3D(width, height, 1)), image_view);

    commands->SetPipelineState(*m_pipeline);

    for (const glm::uvec2& pos : world.chunk_manager().visible_chunks()) {
        const RenderChunk& chunk = world.chunk_manager().render_chunks().find(pos)->second;

        if (!chunk.walls_empty()) {
            const Texture& t = Assets::GetTexture(TextureAsset::Walls);

            commands->SetVertexBufferArray(*chunk.wall_buffer_array);
            commands->SetResource(0, *Renderer::GlobalUniformBuffer());
            commands->SetResource(1, *m_depth_buffer);
            commands->SetResource(2, *t.texture);
            commands->SetResource(3, Assets::GetSampler(t.sampler));

            commands->DrawInstanced(4, 0, chunk.walls_count);
        }

        if (!chunk.blocks_empty()) {
            const Texture& t = Assets::GetTexture(TextureAsset::Tiles);

            commands->SetVertexBufferArray(*chunk.block_buffer_array);
            commands->SetResource(0, *Renderer::GlobalUniformBuffer());
            commands->SetResource(1, *m_depth_buffer);
            commands->SetResource(2, *t.texture);
            commands->SetResource(3, Assets::GetSampler(t.sampler));

            commands->DrawInstanced(4, 0, chunk.blocks_count);
        }
    }
}

void WorldRenderer::terminate() {
    if (m_pipeline) Renderer::Context()->Release(*m_pipeline);
    if (m_depth_buffer) Renderer::Context()->Release(*m_depth_buffer);
    if (m_lightmap_texture) Renderer::Context()->Release(*m_lightmap_texture);
}