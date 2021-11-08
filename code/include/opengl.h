#pragma once
#if defined(APIENTRY) && defined(_WIN32)
#undef APIENTRY
#endif

// This should be only place glad is included
#include <glad/include/glad/glad.h>

#ifdef DEBUG
#define XS_DEBUG_ONLY(x) (x)
namespace xs { void init_debug_messages(); }
#else
#define XS_DEBUG_ONLY(x)
namespace xs { inline void init_debug_messages() {} }
#endif