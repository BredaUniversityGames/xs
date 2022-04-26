#pragma once
#if defined(PLATFORM_PC) || defined(PLATFORM_SWITCH)
	#if defined(APIENTRY) && defined(_WIN32)
	#undef APIENTRY
	#endif
	#include <spdlog/spdlog.h>
#elif defined(PLATFORM_PS5)
	#include <spdlog/fmt/fmt.h>
#endif

namespace xs
{

class log
{
public:
	static void initialize();
	
	template<typename FormatString, typename... Args>
	static void info(const FormatString& fmt, const Args&... args);

	template<typename FormatString, typename... Args>
	static void warn(const FormatString& fmt, const Args&... args);

	template<typename FormatString, typename... Args>
	static void error(const FormatString& fmt, const Args&... args);


};

#if defined(PLATFORM_PC) || defined(PLATFORM_SWITCH)

template<typename FormatString, typename ...Args>
inline void log::info(const FormatString& fmt, const Args & ...args)
{
	spdlog::info(fmt, args...);
}

template<typename FormatString, typename ...Args>
inline void log::warn(const FormatString& fmt, const Args & ...args)
{
	spdlog::warn(fmt, args...);
}

template<typename FormatString, typename ...Args>
inline void log::error(const FormatString& fmt, const Args & ...args)
{
	spdlog::error(fmt, args...);
}

#elif defined(PLATFORM_PS5)

template<typename FormatString, typename ...Args>
inline void log::info(const FormatString& fmt, const Args & ...args)
{
}

template<typename FormatString, typename ...Args>
inline void log::warn(const FormatString& fmt, const Args & ...args)
{
}

template<typename FormatString, typename ...Args>
inline void log::error(const FormatString& fmt, const Args & ...args)
{
}

#endif

}
