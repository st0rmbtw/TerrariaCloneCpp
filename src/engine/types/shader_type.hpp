#ifndef _ENGINE_TYPES_SHADER_TYPE_HPP_
#define _ENGINE_TYPES_SHADER_TYPE_HPP_

#pragma once

#include <cstdint>
#include <LLGL/ShaderFlags.h>

#include "../assert.hpp"

#include "backend.hpp"

class ShaderType {
public:
    enum Value : uint8_t {
        Vertex = 0,
        Fragment,
        Geometry,
        Compute
    };

    ShaderType() = default;
    constexpr ShaderType(Value backend) : m_value(backend) {}

    constexpr operator Value() const { return m_value; }
    explicit operator bool() const = delete;

    [[nodiscard]]
    inline constexpr LLGL::ShaderType ToLLGLType() const {
        switch (m_value) {
            case Value::Vertex: return LLGL::ShaderType::Vertex; break;
            case Value::Fragment: return LLGL::ShaderType::Fragment; break;
            case Value::Geometry: return LLGL::ShaderType::Geometry; break;
            case Value::Compute: return LLGL::ShaderType::Compute; break;
            default: UNREACHABLE();
        };
    }

    [[nodiscard]]
    inline constexpr const char* EntryPoint(RenderBackend backend) const {
        if (backend.IsOpenGL() || backend.IsVulkan()) return nullptr;

        switch (m_value) {
            case Value::Vertex: return "VS"; break;
            case Value::Fragment: return "PS"; break;
            case Value::Geometry: return "GS"; break;
            default: return nullptr;
        };
    }

    [[nodiscard]]
    inline constexpr const char* Profile(RenderBackend backend) const {
        switch (backend) {
        case RenderBackend::OpenGL:
        case RenderBackend::Vulkan: return nullptr;

        case RenderBackend::D3D11: switch (m_value) {
            case Value::Vertex: return "vs_5_0"; break;
            case Value::Fragment: return "ps_5_0"; break;
            case Value::Geometry: return "gs_5_0"; break;
            case Value::Compute: return "cs_5_0"; break;
            default: return nullptr;
        };
        break;

        case RenderBackend::D3D12: switch (m_value) {
            case Value::Vertex: return "vs_5_0"; break;
            case Value::Fragment: return "ps_5_0"; break;
            case Value::Geometry: return "gs_5_0"; break;
            case Value::Compute: return "cs_5_0"; break;
            default: return nullptr;
        };
        break;

        case RenderBackend::Metal: return "1.1"; break;
        }
    }

    [[nodiscard]]
    inline constexpr const char* FileExtension(RenderBackend backend) const {
        switch (backend) {
            case RenderBackend::D3D11:
            case RenderBackend::D3D12: return ".hlsl"; break;
            case RenderBackend::Metal: return ".metal"; break;
            case RenderBackend::OpenGL: switch (m_value) {
                case Value::Vertex:   return ".vert"; break;
                case Value::Fragment: return ".frag"; break;
                case Value::Geometry: return ".geom"; break;
                case Value::Compute: return ".comp"; break;
                default: return nullptr;
            };
            case RenderBackend::Vulkan: switch (m_value) {
                case Value::Vertex:   return ".vert.spv"; break;
                case Value::Fragment: return ".frag.spv"; break;
                case Value::Geometry: return ".geom.spv"; break;
                case Value::Compute: return ".comp.spv"; break;
                default: return nullptr;
            };
            default: return nullptr;
        }
    }

    [[nodiscard]] inline constexpr bool IsVertex() const { return m_value == Value::Vertex; }
    [[nodiscard]] inline constexpr bool IsFragment() const { return m_value == Value::Fragment; }
    [[nodiscard]] inline constexpr bool IsGeometry() const { return m_value == Value::Geometry; }
    [[nodiscard]] inline constexpr bool IsCompute() const { return m_value == Value::Compute; }

private:
    Value m_value = Value::Vertex;
};

#endif