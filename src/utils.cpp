#include <stdio.h>
#include "utils.hpp"

bool FileExists(const char *path) {
#ifdef _WIN32
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