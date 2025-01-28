#pragma once

#ifndef TYPES_DEPTH
#define TYPES_DEPTH

struct Depth {

    Depth() = default;

    Depth(int depth, bool advance = true) :
        value(depth),
        advance(advance) {}

    Depth(int depth, bool advance, bool depth_enabled) :
        value(depth),
        advance(advance),
        depth_enabled(depth_enabled) {}

    int value = -1;
    bool advance = true;
    bool depth_enabled = false;
};

#endif