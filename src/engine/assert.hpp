#ifndef _ENGINE_ASSERT_HPP_
#define _ENGINE_ASSERT_HPP_

#pragma once

#if DEBUG
    #include <stdio.h>
    #include <stdlib.h>
    #define ASSERT(expression, message, ...) \
        if (!(expression)) { \
            fprintf(stderr, "[%s:%d] " message "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
            abort(); \
        }

    #define UNREACHABLE() ASSERT(false, "Reached an unreachable point!")
#else
    #define UNREACHABLE() ((void)0);
    #define ASSERT(expression, message, ...) ((void)0);
#endif

#endif
