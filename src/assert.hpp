#ifndef TERRARIA_ASSERT_H
#define TERRARIA_ASSERT_H

#pragma once

#if DEBUG
    #include <stdio.h>
    #include <stdlib.h>
    #define ASSERT(expression, message, ...) \
        if (!(expression)) { \
            fprintf(stderr, "[%s:%d] " message "\n", __FILE__, __LINE__, __VA_ARGS__); \
            exit(1); \
        }

    #define UNREACHABLE() ASSERT(false, "Reached an unreachable point!")
#else
    #define UNREACHABLE() ((void)0);
    #define ASSERT(expression, message, ...) ((void)0);
#endif

#endif
