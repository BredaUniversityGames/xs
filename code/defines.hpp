#pragma once

#if defined(_WIN32)
    #define UNUSED(x) (void)(x)
#elif defined(NN_NINTENDO_SDK)
    #define offset_of(st, m) ((size_t)&(((st *)0)->m))
    #define UNUSED(x) (void)(x)
#endif

#if defined(PLATFORM_PC) || defined(PLATFORM_MAC) || defined(PLATFORM_LINUX)
#define PLATFORM_DESKTOP
#endif

#if defined(PLATFORM_PC) || defined(PLATFORM_LINUX) || defined(PLATFORM_SWITCH)
#define PLATFORM_OPENGL 
#endif

// Autorelease pool macros for Apple platforms
// These help prevent memory leaks from accumulated autoreleased Objective-C objects
#if defined(PLATFORM_APPLE)
    #define XS_AUTORELEASE_POOL_BEGIN @autoreleasepool {
    #define XS_AUTORELEASE_POOL_END }
#else
    #define XS_AUTORELEASE_POOL_BEGIN
    #define XS_AUTORELEASE_POOL_END
#endif

