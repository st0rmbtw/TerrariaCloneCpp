#include <cstring>
#include <cstdio>
#include "game.hpp"

int main(int argc, char** argv) {
// #if DEBUG
    RenderBackend backend = RenderBackend::D3D11;
    GameConfig config;

    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "--pause") == 0) {
                getchar();
            } else if (strcmp(argv[i], "--backend") == 0) {
                if (i < argc-1) {
                    const char* arg = argv[i + 1];
                    if (strcmp(arg, "vulkan") == 0) {
                        backend = RenderBackend::Vulkan;
                    } else if (strcmp(arg, "d3d12") == 0) {
                        backend = RenderBackend::D3D12;
                    } else if (strcmp(arg, "d3d11") == 0) {
                        backend = RenderBackend::D3D11;
                    } else if (strcmp(arg, "metal") == 0) {
                        backend = RenderBackend::Metal;
                    } else if (strcmp(arg, "opengl") == 0) {
                        backend = RenderBackend::OpenGL;
                    } else {
                        printf("Unknown render backend: %s. Available render backends: vulkan, d3d12, d3d11, metal, opengl.\n", arg);
                        return 1;
                    }
                }
            } else if (strcmp(argv[i], "--vsync") == 0) {
                config.vsync = true;
            } else if (strcmp(argv[i], "--fullscreen") == 0) {
                config.fullscreen = true;
            }
        }
    }
// #endif

    if (!Game::Init(backend, config)) return -1;
    Game::Run();
    Game::Destroy();

    return 0;
}