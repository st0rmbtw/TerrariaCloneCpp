#pragma once

#ifndef TERRARIA_RENDERER_UTILS_HPP
#define TERRARIA_RENDERER_UTILS_HPP

#include <LLGL/LLGL.h>
#include <LLGL/Utils/Utility.h>
#include "renderer/renderer.hpp"

template <typename Container>
std::size_t GetArraySize(const Container& container)
{
    return (container.size() * sizeof(typename Container::value_type));
}

template <typename T, std::size_t N>
std::size_t GetArraySize(const T (&container)[N])
{
    return (N * sizeof(T));
}

template <typename Container>
LLGL::Buffer* CreateVertexBuffer(const Container& vertices, const LLGL::VertexFormat& vertexFormat)
{
    LLGL::BufferDescriptor bufferDesc = LLGL::VertexBufferDesc(GetArraySize(vertices), vertexFormat);
    bufferDesc.debugName = "VertexBuffer";
    return Renderer::Context()->CreateBuffer(bufferDesc, &vertices[0]);
}

template <typename Container>
LLGL::Buffer* CreateIndexBuffer(const Container& indices, const LLGL::Format format)
{
    LLGL::BufferDescriptor bufferDesc = LLGL::IndexBufferDesc(GetArraySize(indices), format);
    bufferDesc.debugName = "IndexBuffer";
    return Renderer::Context()->CreateBuffer(bufferDesc, &indices[0]);
}

#endif