#include <cstring>
#include <cstdio>
#include <string>
#include <SGE/defines.hpp>
#include <fmt/base.h>

#include "app.hpp"

inline void print_render_backends() {
    #if SGE_PLATFORM_WINDOWS
        fmt::println("Available render backends: d3d11, d3d12, opengl, vulkan.");
    #elif SGE_PLATFORM_MACOS
        fmt::println("Available render backends: metal, opengl, vulkan.");
    #elif SGE_PLATFORM_LINUX
        fmt::println("Available render backends: opengl, vulkan.");
    #endif
}

#define str_eq(a, b) strcmp(a, b) == 0

int main(int argc, char** argv) {
#if SGE_PLATFORM_WINDOWS
    sge::RenderBackend backend = sge::RenderBackend::D3D11;
#elif SGE_PLATFORM_MACOS
    sge::RenderBackend backend = sge::RenderBackend::Metal;
#else
    sge::RenderBackend backend = sge::RenderBackend::Vulkan;
#endif
    AppConfig config;

    int16_t world_width = 200;
    int16_t world_height = 500;

    for (int i = 1; i < argc; i++) {
        if (str_eq(argv[i], "--wait-key")) {
            fmt::println("Press any key to continue...");
            getchar();
        } else if (str_eq(argv[i], "--backend")) {
            if (i >= argc-1) {
                fmt::print("Specify a render backend. ");
                print_render_backends();
                return 1;
            }

            const char* arg = argv[i + 1];

            if (str_eq(arg, "vulkan")) {
                backend = sge::RenderBackend::Vulkan;
            } else

            #ifdef SGE_PLATFORM_WINDOWS
            if (str_eq(arg, "d3d12")) {
                backend = sge::RenderBackend::D3D12;
            } else

            if (str_eq(arg, "d3d11")) {
                backend = sge::RenderBackend::D3D11;
            } else
            #endif

            #ifdef SGE_PLATFORM_MACOS
            if (str_eq(arg, "metal")) {
                backend = sge::RenderBackend::Metal;
            } else
            #endif

            if (str_eq(arg, "opengl")) {
                backend = sge::RenderBackend::OpenGL;
            } else {
                fmt::print("Unknown render backend: {}. ", arg);
                print_render_backends();
                return 1;
            }
        } else if (str_eq(argv[i], "--vsync")) {
            config.vsync = true;
        } else if (str_eq(argv[i], "--fullscreen")) {
            config.fullscreen = true;
        } else if (str_eq(argv[i], "--samples")) {
            if (i >= argc-1) {
                fmt::println("Specify the number of samples.");
                return 1;
            }

            const char* arg = argv[i + 1];
            config.samples = std::stoul(arg);
        } else if (str_eq(argv[i], "--world-width")) {
            if (i >= argc-1) {
                fmt::println("Specify the width of the world.");
                return 1;
            }

            const char* arg = argv[i + 1];
            world_width = std::stoul(arg);
        } else if (str_eq(argv[i], "--world-height")) {
            if (i >= argc-1) {
                fmt::println("Specify the height of the world.");
                return 1;
            }

            const char* arg = argv[i + 1];
            world_height = std::stoul(arg);
        }
    }

    if (App::Init(backend, config, world_width, world_height)) {
        App::Run();
    }
    App::Destroy();

    return 0;
}