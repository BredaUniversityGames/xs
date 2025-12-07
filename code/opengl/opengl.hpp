#pragma once
#if defined(APIENTRY) && defined(_WIN32)
#undef APIENTRY
#endif

#if defined(PLATFORM_PC) || defined(PLATFORM_SWITCH)
// This should be only place glad is included
#include <glad/include/glad/glad.h>
#endif

#include <string>

#ifdef XS_DEBUG
#define XS_DEBUG_ONLY(x) (x)
namespace xs
{
	void init_debug_messages();
	void gl_label(GLenum type, GLuint name, const std::string& label);
}
#else
#define XS_DEBUG_ONLY(x)
namespace xs
{
	inline void init_debug_messages() {}
	inline void gl_label(GLenum, GLuint, const std::string&) {}
}
#endif
