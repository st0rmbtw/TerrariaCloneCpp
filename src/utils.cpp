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

unsigned long hash_str(const char *str) {
    if (str == nullptr) return 0;

    unsigned long hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}