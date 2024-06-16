#include "renderer/custom_surface.hpp"
#include <LLGL/Platform/NativeHandle.h>

CustomSurface::CustomSurface(GLFWwindow * const window, const LLGL::Extent2D& size) :
    m_size(size),
    m_wnd(window) {}

CustomSurface::~CustomSurface() {
    glfwDestroyWindow(m_wnd);
}

void CustomSurface::ResetPixelFormat() {
    // glfwDestroyWindow(m_wnd);
    // m_wnd = CreateGLFWWindow();
    printf("RESET PIXEL FORMAT CALLED\n");
}

bool CustomSurface::GetNativeHandle(void* nativeHandle, std::size_t nativeHandleSize) {
    auto handle = reinterpret_cast<LLGL::NativeHandle*>(nativeHandle);
#if defined(_WIN32)
    handle->window = glfwGetWin32Window(m_wnd);
#elif defined(__MACH__)
    handle->responder = glfwGetCocoaWindow(m_wnd);
#else
    // TODO
    #if defined(WAYLAND)
    #elif defined(X11)
  #endif
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