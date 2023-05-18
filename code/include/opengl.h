#pragma once
#if defined(APIENTRY) && defined(_WIN32)
#undef APIENTRY
#endif

#if defined(__APPLE__)
#include <OpenGL/gl.h>
#else
// This should be only place glad is included
#include <glad/include/glad/glad.h>
#endif

#include <OpenGL/gl.h>

#ifdef DEBUG
#define XS_DEBUG_ONLY(x) (x)
namespace xs { void init_debug_messages(); }
#else
#define XS_DEBUG_ONLY(x)
namespace xs { inline void init_debug_messages() {} }
#endif
