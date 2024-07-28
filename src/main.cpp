#include <string.h>
#include <stdio.h>
#include "game.hpp"

int main(int argc, char** argv) {
// #if DEBUG
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "--pause") == 0) {
                getchar();
            }
        }
    }
// #endif

    if (!Game::Init()) return -1;
    Game::Run();
    Game::Destroy();

    return 0;
}