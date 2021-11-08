#pragma once
#if defined(APIENTRY) && defined(_WIN32)
#undef APIENTRY
#endif
#include <spdlog/spdlog.h>

namespace xs
{

class log
{
public:
	static void initialize();
	
	template<typename FormatString, typename... Args>
	static void info(const FormatString& fmt, const Args&... args)
	{ spdlog::info(fmt, args...); }

	template<typename FormatString, typename... Args>
	static void warn(const FormatString& fmt, const Args&... args)
	{ spdlog::warn(fmt, args...); }

	template<typename FormatString, typename... Args>
	static void error(const FormatString& fmt, const Args&... args)
	{ spdlog::error(fmt, args...); }
};

}
