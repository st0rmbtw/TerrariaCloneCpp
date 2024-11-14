#include "custom_surface.hpp"

#include <LLGL/Platform/NativeHandle.h>
#include <GLFW/glfw3native.h>

CustomSurface::CustomSurface(GLFWwindow * const window, const LLGL::Extent2D& size) :
    m_size(size),
    m_wnd(window) {}

CustomSurface::CustomSurface(CustomSurface&& other) noexcept :
    m_size(other.m_size),
    m_wnd(other.m_wnd)
{
    other.m_wnd = nullptr;
}

CustomSurface::~CustomSurface() {
    if (m_wnd) glfwDestroyWindow(m_wnd);
}

bool CustomSurface::GetNativeHandle(void* nativeHandle, std::size_t nativeHandleSize) {
    auto* handle = reinterpret_cast<LLGL::NativeHandle*>(nativeHandle);
#if defined(PLATFORM_WINDOWS)
    handle->window = glfwGetWin32Window(m_wnd);
#elif defined(PLATFORM_MACOS)
    handle->responder = glfwGetCocoaWindow(m_wnd);
#elif defined(WAYLAND)
    // TODO
#elif defined(X11)
    // TODO
#endif
    return true;
}

LLGL::Extent2D CustomSurface::GetContentSize() const {
    return m_size;
}

bool CustomSurface::AdaptForVideoMode(LLGL::Extent2D *resolution, bool *fullscreen) {
    m_size = *resolution;
    glfwSetWindowSize(m_wnd, m_size.width, m_size.height);
    return true;
}

LLGL::Display* CustomSurface::FindResidentDisplay() const {
    return LLGL::Display::GetPrimary();
}

bool CustomSurface::ProcessEvents() {
    glfwPollEvents();
    return !glfwWindowShouldClose(m_wnd);
}