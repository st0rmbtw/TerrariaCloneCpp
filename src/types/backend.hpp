#ifndef TYPES_BACKEND
#define TYPES_BACKEND

#pragma once

#include <cstdint>
#include "../assert.hpp"

class RenderBackend {
public:
    enum Value : uint8_t {
        Vulkan = 0,
        D3D11,
        D3D12,
        Metal,
        OpenGL
    };

    RenderBackend() = default;
    constexpr RenderBackend(Value backend) : m_value(backend) {}

    constexpr operator Value() const { return m_value; }
    explicit operator bool() const = delete;

    [[nodiscard]]
    inline constexpr const char* ToString() const {
        switch (m_value) {
            case Value::Vulkan: return "Vulkan";
            break;
            case Value::D3D11: return "Direct3D11";
            break;
            case Value::D3D12: return "Direct3D12";
            break;
            case Value::Metal: return "Metal";
            break;
            case Value::OpenGL: return "OpenGL";
            break;
            default: UNREACHABLE()
        };
    }

    [[nodiscard]]
    inline constexpr const char* AssetFolder() const {
        switch (m_value) {
            case Value::Vulkan: return "assets/shaders/vulkan/";
            break;
            case Value::D3D11: return "assets/shaders/d3d11/";
            break;
            case Value::D3D12: return "assets/shaders/d3d12/";
            break;
            case Value::Metal: return "assets/shaders/metal/";
            break;
            case Value::OpenGL: return "assets/shaders/opengl/";
            break;
            default: UNREACHABLE()
        };
    }

    [[nodiscard]] inline constexpr bool IsVulkan() const { return m_value == Value::Vulkan; }
    [[nodiscard]] inline constexpr bool IsD3D11() const { return m_value == Value::D3D11; }
    [[nodiscard]] inline constexpr bool IsD3D12() const { return m_value == Value::D3D12; }
    [[nodiscard]] inline constexpr bool IsMetal() const { return m_value == Value::Metal; }
    [[nodiscard]] inline constexpr bool IsOpenGL() const { return m_value == Value::OpenGL; }
    
    [[nodiscard]] inline constexpr bool IsGLSL() const { return m_value == Value::OpenGL || m_value == Value::Vulkan; }
    [[nodiscard]] inline constexpr bool IsHLSL() const { return m_value == Value::D3D11 || m_value == Value::D3D12; }

private:
    Value m_value = Value::Vulkan;
};

#endif