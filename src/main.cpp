#include <cstring>
#include <cstdio>
#include "game.hpp"

inline void print_render_backends() {
    #if defined(_WIN32)
        printf("Available render backends: vulkan, d3d12, d3d11, opengl.\n");
    #elif defined(__APPLE__)
        printf("Available render backends: vulkan, metal, opengl.\n");
    #else
        printf("Available render backends: vulkan, opengl.\n");
    #endif
}

#define str_eq(a, b) strcmp(a, b) == 0

int main(int argc, char** argv) {
    RenderBackend backend = RenderBackend::OpenGL;
    GameConfig config;

    for (int i = 1; i < argc; i++) {
        if (str_eq(argv[i], "--pause")) {
            getchar();
        } else if (str_eq(argv[i], "--backend")) {
            if (i >= argc-1) {
                printf("Specify a render backend. ");
                print_render_backends();
                return 1;
            }

            const char* arg = argv[i + 1];

            if (str_eq(arg, "vulkan")) {
                backend = RenderBackend::Vulkan;
            } else

            #ifdef _WIN32
            if (str_eq(arg, "d3d12")) {
                backend = RenderBackend::D3D12;
            } else
            
            if (str_eq(arg, "d3d11")) {
                backend = RenderBackend::D3D11;
            } else
            #endif

            #ifdef __APPLE__
            if (str_eq(arg, "metal")) {
                backend = RenderBackend::Metal;
            } else
            #endif

            if (str_eq(arg, "opengl")) {
                backend = RenderBackend::OpenGL;
            } else {
                printf("Unknown render backend: \"%s\". ", arg);
                print_render_backends();
                return 1;
            }
        } else if (str_eq(argv[i], "--vsync")) {
            config.vsync = true;
        } else if (str_eq(argv[i], "--fullscreen")) {
            config.fullscreen = true;
        }
    }

    if (!Game::Init(backend, config)) return -1;
    Game::Run();
    Game::Destroy();

    return 0;
}