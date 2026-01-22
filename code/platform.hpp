#pragma once
#include "defines.hpp"

#if defined(PLATFORM_PS5)
    #include "../platforms/prospero/code/platform_ps5.hpp"
#elif defined(PLATFORM_OPENGL)
    #include "opengl/platform_opengl.hpp"
#elif defined(PLATFORM_APPLE)
    #include "../platforms/apple/platform_apple.hpp"
#endif