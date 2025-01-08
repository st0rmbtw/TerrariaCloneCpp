#ifndef RENDERER_UTILS_HPP
#define RENDERER_UTILS_HPP

#pragma once

#include <LLGL/Utils/Utility.h>
#include "renderer.hpp"

template <typename Container>
inline std::size_t GetArraySize(const Container& container)
{
    return (container.size() * sizeof(typename Container::value_type));
}

template <typename T, std::size_t N>
inline std::size_t GetArraySize(const T (&)[N])
{
    return (N * sizeof(T));
}

template <typename Container>
inline LLGL::Buffer* CreateVertexBuffer(const Container& vertices, const LLGL::VertexFormat& vertexFormat, const char* debug_name = nullptr)
{
    LLGL::BufferDescriptor bufferDesc = LLGL::VertexBufferDesc(GetArraySize(vertices), vertexFormat);
    bufferDesc.debugName = debug_name;
    return Renderer::Context()->CreateBuffer(bufferDesc, &vertices[0]);
}

inline LLGL::Buffer* CreateVertexBuffer(size_t size, const LLGL::VertexFormat& vertexFormat, const char* debug_name = nullptr)
{
    LLGL::BufferDescriptor bufferDesc = LLGL::VertexBufferDesc(size, vertexFormat);
    bufferDesc.debugName = debug_name;
    return Renderer::Context()->CreateBuffer(bufferDesc, nullptr);
}

inline LLGL::Buffer* CreateVertexBufferInit(size_t size, const void* data, const LLGL::VertexFormat& vertexFormat, const char* debug_name = nullptr)
{
    LLGL::BufferDescriptor bufferDesc = LLGL::VertexBufferDesc(size, vertexFormat);
    bufferDesc.debugName = debug_name;
    return Renderer::Context()->CreateBuffer(bufferDesc, data);
}

template <typename Container>
inline LLGL::Buffer* CreateIndexBuffer(const Container& indices, const LLGL::Format format, const char* debug_name = nullptr)
{
    LLGL::BufferDescriptor bufferDesc = LLGL::IndexBufferDesc(GetArraySize(indices), format);
    bufferDesc.debugName = debug_name;
    return Renderer::Context()->CreateBuffer(bufferDesc, &indices[0]);
}

inline LLGL::Buffer* CreateConstantBuffer(const size_t size, const char* debug_name = nullptr)
{
    LLGL::BufferDescriptor bufferDesc = LLGL::ConstantBufferDesc(size);
    bufferDesc.debugName = debug_name;
    return Renderer::Context()->CreateBuffer(bufferDesc);
}

#endif