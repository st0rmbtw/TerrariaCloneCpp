#pragma once

#ifndef TERRARIA_CUSTOM_SURFACE_HPP
#define TERRARIA_CUSTOM_SURFACE_HPP

#if defined(_WIN32)
    #define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__MACH__)
    #define GLFW_EXPOSE_NATIVE_COCOA
#else
    #if defined(WAYLAND)
        #define GLFW_EXPOSE_NATIVE_WAYLAND
    #elif defined(X11)
        #define GLFW_EXPOSE_NATIVE_X11
  #endif
#endif

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <LLGL/LLGL.h>

class CustomSurface : public LLGL::Surface {
public:
    CustomSurface(GLFWwindow * const window, const LLGL::Extent2D& size);
    ~CustomSurface();

    bool GetNativeHandle(void* nativeHandle, std::size_t nativeHandleSize) override;
    LLGL::Extent2D GetContentSize() const override;
    bool AdaptForVideoMode(LLGL::Extent2D* resolution, bool* fullscreen) override;
    void ResetPixelFormat() override;
    LLGL::Display* FindResidentDisplay() const override;

    bool ProcessEvents();
private:
    LLGL::Extent2D m_size;
    GLFWwindow* m_wnd = nullptr;

};

#endif