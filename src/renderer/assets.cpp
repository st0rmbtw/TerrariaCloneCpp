#include "renderer/assets.hpp"
#include "renderer/renderer.hpp"

LLGL::VertexFormat SpriteVertexFormat() {
    static LLGL::VertexFormat vertexFormat;

    if (vertexFormat.attributes.empty()) {
#if defined(BACKEND_OPENGL)
        vertexFormat.AppendAttribute({"a_transform_col_0", LLGL::Format::RGBA32Float});
        vertexFormat.AppendAttribute({"a_transform_col_1", LLGL::Format::RGBA32Float});
        vertexFormat.AppendAttribute({"a_transform_col_2", LLGL::Format::RGBA32Float});
        vertexFormat.AppendAttribute({"a_transform_col_3", LLGL::Format::RGBA32Float});
        vertexFormat.AppendAttribute({"a_uv_offset_scale", LLGL::Format::RGBA32Float});
        vertexFormat.AppendAttribute({"a_color", LLGL::Format::RGBA32Float});
#elif defined(BACKEND_D3D11)
        vertexFormat.AppendAttribute({"Transform", 0, LLGL::Format::RGBA32Float});
        vertexFormat.AppendAttribute({"Transform", 1, LLGL::Format::RGBA32Float});
        vertexFormat.AppendAttribute({"Transform", 2, LLGL::Format::RGBA32Float});
        vertexFormat.AppendAttribute({"Transform", 3, LLGL::Format::RGBA32Float});
        vertexFormat.AppendAttribute({"UvOffsetScale", LLGL::Format::RGBA32Float});
        vertexFormat.AppendAttribute({"Color", LLGL::Format::RGBA32Float});
#elif defined(BACKEND_METAL)
        vertexFormat.AppendAttribute({"transform0", LLGL::Format::RGBA32Float});
        vertexFormat.AppendAttribute({"transform1", LLGL::Format::RGBA32Float});
        vertexFormat.AppendAttribute({"transform2", LLGL::Format::RGBA32Float});
        vertexFormat.AppendAttribute({"transform3", LLGL::Format::RGBA32Float});
        vertexFormat.AppendAttribute({"uv_offset_scale", LLGL::Format::RGBA32Float});
        vertexFormat.AppendAttribute({"color", LLGL::Format::RGBA32Float});
#endif
        vertexFormat.SetStride(sizeof(SpriteVertex));
    }

    return vertexFormat;
}
