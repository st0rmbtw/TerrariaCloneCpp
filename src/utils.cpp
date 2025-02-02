#include "utils.hpp"

#include <stdio.h>

#include "engine/defines.hpp"

bool FileExists(const char *path) {
#ifdef PLATFORM_WINDOWS
    FILE *file = NULL;
    fopen_s(&file, path, "r");
#else
    FILE *file = fopen(path, "r");
#endif

    bool exists = file != NULL;

    if (file != NULL) {
        fclose(file);
    }

    return exists;
}