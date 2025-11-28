#pragma once

#if defined(_WIN32)	
    #define UNUSED(x) (void)(x)
#elif defined(NN_NINTENDO_SDK)
    #define offset_of(st, m) ((size_t)&(((st *)0)->m))
    #define UNUSED(x) (void)(x)
#endif

#if defined(PLATFORM_PC) || defined(PLATFORM_MAC)
#define PLATFORM_DESKTOP
#endif

