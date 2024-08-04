#include "assets.hpp"
#include "renderer.hpp"

const LLGL::VertexFormat& SpriteVertexFormat() {
    static LLGL::VertexFormat vertexFormat;

    if (!vertexFormat.attributes.empty()) return vertexFormat;

    vertexFormat.SetStride(sizeof(SpriteVertex));

    switch (Renderer::Backend()) {
        case RenderBackend::Vulkan:
        case RenderBackend::OpenGL: {
            vertexFormat.AppendAttribute({"a_transform_col_0", LLGL::Format::RGBA32Float});
            vertexFormat.AppendAttribute({"a_transform_col_1", LLGL::Format::RGBA32Float});
            vertexFormat.AppendAttribute({"a_transform_col_2", LLGL::Format::RGBA32Float});
            vertexFormat.AppendAttribute({"a_transform_col_3", LLGL::Format::RGBA32Float});
            vertexFormat.AppendAttribute({"a_uv_offset_scale", LLGL::Format::RGBA32Float});
            vertexFormat.AppendAttribute({"a_color", LLGL::Format::RGBA32Float});
            vertexFormat.AppendAttribute({"a_outline_color", LLGL::Format::RGBA32Float});
            vertexFormat.AppendAttribute({"a_outline_thickness", LLGL::Format::R32Float});
            vertexFormat.AppendAttribute({"a_has_texture", LLGL::Format::R32SInt});
            vertexFormat.AppendAttribute({"a_is_ui", LLGL::Format::R32SInt});
        }
        break;
        case RenderBackend::D3D11:
        case RenderBackend::D3D12: {
            vertexFormat.AppendAttribute({"Transform", 0, LLGL::Format::RGBA32Float});
            vertexFormat.AppendAttribute({"Transform", 1, LLGL::Format::RGBA32Float});
            vertexFormat.AppendAttribute({"Transform", 2, LLGL::Format::RGBA32Float});
            vertexFormat.AppendAttribute({"Transform", 3, LLGL::Format::RGBA32Float});
            vertexFormat.AppendAttribute({"UvOffsetScale", LLGL::Format::RGBA32Float});
            vertexFormat.AppendAttribute({"Color", LLGL::Format::RGBA32Float});
            vertexFormat.AppendAttribute({"OutlineColor", LLGL::Format::RGBA32Float});
            vertexFormat.AppendAttribute({"OutlineThickness", LLGL::Format::R32Float});
            vertexFormat.AppendAttribute({"HasTexture", LLGL::Format::R32SInt});
            vertexFormat.AppendAttribute({"IsUI", LLGL::Format::R32SInt});        
        }
        break;
        case RenderBackend::Metal: {
            vertexFormat.AppendAttribute({"transform0", LLGL::Format::RGBA32Float});
            vertexFormat.AppendAttribute({"transform1", LLGL::Format::RGBA32Float});
            vertexFormat.AppendAttribute({"transform2", LLGL::Format::RGBA32Float});
            vertexFormat.AppendAttribute({"transform3", LLGL::Format::RGBA32Float});
            vertexFormat.AppendAttribute({"uv_offset_scale", LLGL::Format::RGBA32Float});
            vertexFormat.AppendAttribute({"color", LLGL::Format::RGBA32Float});
            vertexFormat.AppendAttribute({"outline_color", LLGL::Format::RGBA32Float});
            vertexFormat.AppendAttribute({"outline_thickness", LLGL::Format::R32Float});
            vertexFormat.AppendAttribute({"has_texture", LLGL::Format::R32SInt});
            vertexFormat.AppendAttribute({"is_ui", LLGL::Format::R32SInt});
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
            vertexFormat.AppendAttribute({"a_position", LLGL::Format::RG32Float});
            vertexFormat.AppendAttribute({"a_atlas_pos", LLGL::Format::RG32Float});
            vertexFormat.AppendAttribute({"a_tile_id", LLGL::Format::R32UInt});
            vertexFormat.AppendAttribute({"a_tile_type", LLGL::Format::R32UInt});
        }
        break;
        case RenderBackend::D3D11:
        case RenderBackend::D3D12: {
            vertexFormat.AppendAttribute({"Position", LLGL::Format::RG32Float});
            vertexFormat.AppendAttribute({"AtlasPos", LLGL::Format::RG32Float});
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