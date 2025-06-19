#include <cstring>
#include <cstdio>
#include <string>
#include <SGE/defines.hpp>
#include "game.hpp"

inline void print_render_backends() {
    #if SGE_PLATFORM_WINDOWS
        printf("Available render backends: d3d11, d3d12, opengl, vulkan.\n");
    #elif SGE_PLATFORM_MACOS
        printf("Available render backends: metal, opengl, vulkan.\n");
    #elif SGE_PLATFORM_LINUX
        printf("Available render backends: opengl, vulkan.\n");
    #endif
}

#define str_eq(a, b) strcmp(a, b) == 0

int main(int argc, char** argv) {
#if SGE_PLATFORM_WINDOWS
    sge::RenderBackend backend = sge::RenderBackend::D3D11;
#elif SGE_PLATFORM_MACOS
    sge::RenderBackend backend = sge::RenderBackend::Metal;
#else
    sge::RenderBackend backend = sge::RenderBackend::OpenGL;
#endif
    AppConfig config;

    for (int i = 1; i < argc; i++) {
        if (str_eq(argv[i], "--pause")) {
            printf("Initialization is paused. Press any key to continue...\n");
            getchar();
        } else if (str_eq(argv[i], "--backend")) {
            if (i >= argc-1) {
                printf("Specify a render backend. ");
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
                printf("Unknown render backend: \"%s\". ", arg);
                print_render_backends();
                return 1;
            }
        } else if (str_eq(argv[i], "--vsync")) {
            config.vsync = true;
        } else if (str_eq(argv[i], "--fullscreen")) {
            config.fullscreen = true;
        } else if (str_eq(argv[i], "--samples")) {
            if (i >= argc-1) {
                printf("Specify the number of samples.\n");
                return 1;
            }

            const char* arg = argv[i + 1];
            config.samples = std::stoul(arg);
        }
    }

    if (Game::Init(backend, config)) {
        Game::Run();
    }
    Game::Destroy();

    return 0;
}