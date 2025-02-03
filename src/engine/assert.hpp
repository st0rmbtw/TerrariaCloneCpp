#ifndef _ENGINE_ASSERT_HPP_
#define _ENGINE_ASSERT_HPP_

#pragma once

#if DEBUG
    #include <stdio.h>
    #include <stdlib.h>
    #define ASSERT(expression, message, ...)                                                    \
        do {                                                                                    \
            if (!(expression)) {                                                                \
                fprintf(stderr, "[%s:%d] " message "\n", __FILE__, __LINE__, ##__VA_ARGS__);    \
                abort();                                                                        \
            }                                                                                   \
        } while (0)
        
    #define UNREACHABLE() ASSERT(false, "Reached an unreachable point!")
#else
    #define ASSERT(expression, message, ...) ((void)0)

    #if defined(__GNUC__) || defined(__clang__)
        #define UNREACHABLE() __builtin_unreachable()
    #elif defiend(_MSC_VER)
        #define UNREACHABLE() __assume(false)
    #else
        #error "Unknown compiler; can't define UNREACHABLE"
    #endif

#endif



#endif
