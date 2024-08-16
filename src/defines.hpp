#if defined(__APPLE__)
    #define MACOS_AUTORELEASEPOOL_OPEN @autoreleasepool {
    #define MACOS_AUTORELEASEPOOL_CLOSE }
#else
    #define MACOS_AUTORELEASEPOOL_OPEN
    #define MACOS_AUTORELEASEPOOL_CLOSE
#endif