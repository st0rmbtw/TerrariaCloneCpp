#ifndef _ENGINE_TYPES_WINDOW_SETTINGS_HPP_
#define _ENGINE_TYPES_WINDOW_SETTINGS_HPP_

#include <stdint.h>

struct WindowSettings {
    uint32_t width = 1280;
    uint32_t height = 720;
    bool fullscreen = false;
    bool hidden = false;
};

#endif