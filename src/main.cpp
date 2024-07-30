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

int main(int argc, char** argv) {
    RenderBackend backend = RenderBackend::D3D11;
    GameConfig config;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--pause") == 0) {
            getchar();
        } else if (strcmp(argv[i], "--backend") == 0) {
            if (i >= argc-1) {
                printf("Specify a render backend. ");
                print_render_backends();
                return 1;
            }

            const char* arg = argv[i + 1];

            if (strcmp(arg, "vulkan") == 0) {
                backend = RenderBackend::Vulkan;
            } else

            #ifdef _WIN32
            if (strcmp(arg, "d3d12") == 0) {
                backend = RenderBackend::D3D12;
            } else
            
            if (strcmp(arg, "d3d11") == 0) {
                backend = RenderBackend::D3D11;
            } else
            #endif

            #ifdef __APPLE__
            if (strcmp(arg, "metal") == 0) {
                backend = RenderBackend::Metal;
            } else
            #endif

            if (strcmp(arg, "opengl") == 0) {
                backend = RenderBackend::OpenGL;
            } else {
                printf("Unknown render backend: \"%s\". ", arg);
                print_render_backends();
                return 1;
            }
        } else if (strcmp(argv[i], "--vsync") == 0) {
            config.vsync = true;
        } else if (strcmp(argv[i], "--fullscreen") == 0) {
            config.fullscreen = true;
        }
    }

    if (!Game::Init(backend, config)) return -1;
    Game::Run();
    Game::Destroy();

    return 0;
}