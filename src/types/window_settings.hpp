#ifndef TYPES_WINDOW_SETTINGS
#define TYPES_WINDOW_SETTINGS

#include <stdint.h>

struct WindowSettings {
    uint32_t width = 1280;
    uint32_t height = 720;
    bool fullscreen = false;
    bool hidden = false;
};

#endif