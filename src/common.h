#ifndef COMMON_H
#define COMMON_H

#pragma once

#if DEBUG
    #include <stdio.h>
    #include <stdlib.h>
    #define ASSERT(expression, message) \
        if (!(expression)) { \
            fprintf(stderr, "[%s:%d] %s\n", __FILE__, __LINE__, message); \
            exit(1); \
        }

    #define UNREACHABLE() ASSERT(false, "Reached an unreachable point!")
#else
    #define UNREACHABLE() ((void)0);
    #define ASSERT(expression, message) ((void)0);
#endif

#endif
