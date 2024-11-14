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

#define FORCE_INLINE inline __attribute__((always_inline))