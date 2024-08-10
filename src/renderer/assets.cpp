#include "assets.hpp"
#include "particle_renderer.hpp"
#include "renderer.hpp"

const LLGL::VertexFormat& SpriteVertexFormat() {
    static LLGL::VertexFormat vertexFormat;

    if (!vertexFormat.attributes.empty()) return vertexFormat;

    vertexFormat.SetStride(sizeof(SpriteVertex));

    switch (Renderer::Backend()) {
        case RenderBackend::Vulkan:
        case RenderBackend::OpenGL: {
            vertexFormat.AppendAttribute({"a_position", LLGL::Format::RGB32Float});
            vertexFormat.AppendAttribute({"a_rotation", LLGL::Format::RGBA32Float});
            vertexFormat.AppendAttribute({"a_size", LLGL::Format::RG32Float});
            vertexFormat.AppendAttribute({"a_offset", LLGL::Format::RG32Float});
            vertexFormat.AppendAttribute({"a_uv_offset_scale", LLGL::Format::RGBA32Float});
            vertexFormat.AppendAttribute({"a_color", LLGL::Format::RGBA32Float});
            vertexFormat.AppendAttribute({"a_outline_color", LLGL::Format::RGBA32Float});
            vertexFormat.AppendAttribute({"a_outline_thickness", LLGL::Format::R32Float});
            vertexFormat.AppendAttribute({"a_has_texture", LLGL::Format::R32SInt});
            vertexFormat.AppendAttribute({"a_is_ui", LLGL::Format::R32SInt});
            vertexFormat.AppendAttribute({"a_is_nonscale", LLGL::Format::R32SInt});
        }
        break;
        case RenderBackend::D3D11:
        case RenderBackend::D3D12: {
            vertexFormat.AppendAttribute({"Position", LLGL::Format::RGB32Float});
            vertexFormat.AppendAttribute({"Rotation", LLGL::Format::RGBA32Float});
            vertexFormat.AppendAttribute({"Size", LLGL::Format::RG32Float});
            vertexFormat.AppendAttribute({"Offset", LLGL::Format::RG32Float});
            vertexFormat.AppendAttribute({"UvOffsetScale", LLGL::Format::RGBA32Float});
            vertexFormat.AppendAttribute({"Color", LLGL::Format::RGBA32Float});
            vertexFormat.AppendAttribute({"OutlineColor", LLGL::Format::RGBA32Float});
            vertexFormat.AppendAttribute({"OutlineThickness", LLGL::Format::R32Float});
            vertexFormat.AppendAttribute({"HasTexture", LLGL::Format::R32SInt});
            vertexFormat.AppendAttribute({"IsUI", LLGL::Format::R32SInt});        
            vertexFormat.AppendAttribute({"IsNonScale", LLGL::Format::R32SInt});        
        }
        break;
        case RenderBackend::Metal: {
            // TODO
        }
        break;
        default: UNREACHABLE();
    }

    return vertexFormat;
}

const LLGL::VertexFormat& GlyphVertexFormat() {
    static LLGL::VertexFormat vertexFormat;

    if (!vertexFormat.attributes.empty()) return vertexFormat;

    vertexFormat.SetStride(sizeof(SpriteVertex));

    switch (Renderer::Backend()) {
        case RenderBackend::Vulkan:
        case RenderBackend::OpenGL: {
            vertexFormat.AppendAttribute({"a_color", LLGL::Format::RGB32Float});
            vertexFormat.AppendAttribute({"a_position", LLGL::Format::RGB32Float});
            vertexFormat.AppendAttribute({"a_uv", LLGL::Format::RG32Float});
            vertexFormat.AppendAttribute({"a_is_ui", LLGL::Format::R32SInt});
        }
        break;
        case RenderBackend::D3D11:
        case RenderBackend::D3D12: {
            vertexFormat.AppendAttribute({"Color", LLGL::Format::RGB32Float});
            vertexFormat.AppendAttribute({"Position", LLGL::Format::RGB32Float});
            vertexFormat.AppendAttribute({"UV", LLGL::Format::RG32Float});
            vertexFormat.AppendAttribute({"IsUI", LLGL::Format::R32SInt});
        }
        break;
        case RenderBackend::Metal: {
            // TODO
        }
        break;
        default: UNREACHABLE();
    }

    return vertexFormat;
}

const LLGL::VertexFormat& TilemapVertexFormat() {
    static LLGL::VertexFormat vertexFormat;

    if (!vertexFormat.attributes.empty()) return vertexFormat;

    switch (Renderer::Backend()) {
        case RenderBackend::Vulkan:
        case RenderBackend::OpenGL: {
            vertexFormat.AppendAttribute({"a_uv_size", LLGL::Format::RGBA32Float});
            vertexFormat.AppendAttribute({"a_position", LLGL::Format::RG32Float});
            vertexFormat.AppendAttribute({"a_world_pos", LLGL::Format::RG32Float});
            vertexFormat.AppendAttribute({"a_tile_id", LLGL::Format::R32UInt});
            vertexFormat.AppendAttribute({"a_tile_type", LLGL::Format::R32UInt});
        }
        break;
        case RenderBackend::D3D11:
        case RenderBackend::D3D12: {
            vertexFormat.AppendAttribute({"UvSize", LLGL::Format::RGBA32Float});
            vertexFormat.AppendAttribute({"Position", LLGL::Format::RG32Float});
            vertexFormat.AppendAttribute({"WorldPos", LLGL::Format::RG32Float});
            vertexFormat.AppendAttribute({"TileId", LLGL::Format::R32UInt});
            vertexFormat.AppendAttribute({"TileType", LLGL::Format::R32UInt});
        }
        break;
        case RenderBackend::Metal: // TODO
        break;
        default: UNREACHABLE()
    }

    return vertexFormat;
}

const LLGL::VertexFormat& BackgroundVertexFormat() {
    static LLGL::VertexFormat vertexFormat;

    if (!vertexFormat.attributes.empty()) return vertexFormat;

    switch (Renderer::Backend()) {
        case RenderBackend::Vulkan:
        case RenderBackend::OpenGL: {
            vertexFormat.AppendAttribute({"a_position", LLGL::Format::RG32Float});
            vertexFormat.AppendAttribute({"a_uv", LLGL::Format::RG32Float});
            vertexFormat.AppendAttribute({"a_size", LLGL::Format::RG32Float});
            vertexFormat.AppendAttribute({"a_tex_size", LLGL::Format::RG32Float});
            vertexFormat.AppendAttribute({"a_speed", LLGL::Format::RG32Float});
            vertexFormat.AppendAttribute({"a_nonscale", LLGL::Format::R32SInt});
        }
        break;
        case RenderBackend::D3D11:
        case RenderBackend::D3D12: {
            vertexFormat.AppendAttribute({"Position", LLGL::Format::RG32Float});
            vertexFormat.AppendAttribute({"Uv", LLGL::Format::RG32Float});
            vertexFormat.AppendAttribute({"Size", LLGL::Format::RG32Float});
            vertexFormat.AppendAttribute({"TexSize", LLGL::Format::RG32Float});
            vertexFormat.AppendAttribute({"Speed", LLGL::Format::RG32Float});
            vertexFormat.AppendAttribute({"NonScale", LLGL::Format::R32SInt});
        }
        break;
        case RenderBackend::Metal: // TODO
        break;
        default: UNREACHABLE()
    }

    return vertexFormat;
}

const LLGL::VertexFormat& ParticleVertexFormat() {
    static LLGL::VertexFormat vertexFormat;

    if (!vertexFormat.attributes.empty()) return vertexFormat;

    switch (Renderer::Backend()) {
        case RenderBackend::Vulkan:
        case RenderBackend::OpenGL: {
            vertexFormat.AppendAttribute({ "a_position",      LLGL::Format::RG32Float, 0, 0,                                      sizeof(ParticleVertex), 0 });
            vertexFormat.AppendAttribute({ "a_inv_tex_size",  LLGL::Format::RG32Float, 1, offsetof(ParticleVertex, inv_tex_size), sizeof(ParticleVertex), 0 });
            vertexFormat.AppendAttribute({ "a_tex_size",      LLGL::Format::RG32Float, 2, offsetof(ParticleVertex, tex_size),     sizeof(ParticleVertex), 0 });
        }
        break;
        case RenderBackend::D3D11:
        case RenderBackend::D3D12: {
            vertexFormat.AppendAttribute({ "Position",   LLGL::Format::RG32Float, 0, 0,                                      sizeof(ParticleVertex), 0 });
            vertexFormat.AppendAttribute({ "InvTexSize", LLGL::Format::RG32Float, 1, offsetof(ParticleVertex, inv_tex_size), sizeof(ParticleVertex), 0 });
            vertexFormat.AppendAttribute({ "TexSize",    LLGL::Format::RG32Float, 2, offsetof(ParticleVertex, tex_size),     sizeof(ParticleVertex), 0 });
        }
        break;
        case RenderBackend::Metal: // TODO
        break;
        default: UNREACHABLE()
    }

    return vertexFormat;
}

const LLGL::VertexFormat& ParticleInstanceFormat() {
    static LLGL::VertexFormat vertexFormat;

    if (!vertexFormat.attributes.empty()) return vertexFormat;

    switch (Renderer::Backend()) {
        case RenderBackend::Vulkan:
        case RenderBackend::OpenGL: {
            vertexFormat.AppendAttribute({ "i_uv",    LLGL::Format::RG32Float, 1, offsetof(ParticleInstance, uv),    sizeof(ParticleInstance), 1, 1});
            vertexFormat.AppendAttribute({ "i_depth", LLGL::Format::R32Float,  2, offsetof(ParticleInstance, depth), sizeof(ParticleInstance), 1, 1});
        }
        break;
        case RenderBackend::D3D11:
        case RenderBackend::D3D12: {
            vertexFormat.AppendAttribute({ "I_UV",    LLGL::Format::RG32Float, 1, offsetof(ParticleInstance, uv),    sizeof(ParticleInstance), 1, 1 });
            vertexFormat.AppendAttribute({ "I_Depth", LLGL::Format::R32Float,  2, offsetof(ParticleInstance, depth), sizeof(ParticleInstance), 1, 1 });
        }
        break;
        case RenderBackend::Metal: // TODO
        break;
        default: UNREACHABLE()
    }

    return vertexFormat;
}