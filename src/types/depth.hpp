#pragma once

#ifndef TYPES_DEPTH
#define TYPES_DEPTH

struct Depth {

    Depth() = default;

    Depth(int depth, bool advance = true) :
        value(depth),
        advance(advance) {}

    int value = -1;
    bool advance = true;
};

#endif