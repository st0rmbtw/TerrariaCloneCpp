#ifndef _ENGINE_DEFINES_HPP_
#define _ENGINE_DEFINES_HPP_

#if defined(__APPLE__)
    #define MACOS_AUTORELEASEPOOL_OPEN @autoreleasepool {
    #define MACOS_AUTORELEASEPOOL_CLOSE }
#else
    #define MACOS_AUTORELEASEPOOL_OPEN
    #define MACOS_AUTORELEASEPOOL_CLOSE
#endif

#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__NT__)
    #define PLATFORM_WINDOWS
#elif defined(__APPLE__)
    #define PLATFORM_MACOS
#elif defined(__linux__)
    #define PLATFORM_LINUX
#else
    #error "Unknown platform"
#endif

#if defined(__GNUC__) || defined(__clang__)
    #define FORCE_INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
    #define FORCE_INLINE __forceinline
#else
    #error "Unknown compiler; can't define FORCE_INLINE"
#endif

#if defined(__GNUC__) || defined(__clang__)
    #define ALIGN(x) __attribute__ ((aligned(x)))
#elif defined(_MSC_VER)
    #define ALIGN(x) __declspec(align(x))
#else
    #error "Unknown compiler; can't define ALIGN"
#endif

#if defined(TRACY_ENABLE)
    #define INLINE
#else
    #define INLINE inline
#endif



#endif