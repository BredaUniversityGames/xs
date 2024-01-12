#pragma once

#include <cassert>
#define FMT_HEADER_ONLY
#include <fmt/core.h>

#if defined(PLATFORM_PC) || defined(PLATFORM_SWITCH) || defined(PLATFORM_PROSPERO)
    #define USE_LOG_COLOR
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

    template<typename FormatString, typename... Args>
    static void critical(const FormatString& fmt, const Args&... args);

private:
    
#ifdef USE_LOG_COLOR
	static constexpr auto magenta = "\033[35m";
	static constexpr auto green = "\033[32m";
	static constexpr auto red = "\033[31m";
    static constexpr auto yellow = "\033[33m";
	static constexpr auto reset = "\033[0m";
#else
    static constexpr auto magenta = "";
    static constexpr auto green = "";
    static constexpr auto red = "";
    static constexpr auto yellow = "";
    static constexpr auto reset = "";
#endif
};

template<typename FormatString, typename ...Args>
inline void log::info(const FormatString& format, const Args & ...args)
{
	printf("[%sinfo%s] ", green, reset);
	fmt::print(fmt::runtime(format), args...);
	printf("\n");
}

template<typename FormatString, typename ...Args>
inline void log::warn(const FormatString& format, const Args & ...args)
{
	printf("[%swarn%s] ", magenta, reset);
    fmt::print(fmt::runtime(format), args...);
	printf("\n");
}

template<typename FormatString, typename ...Args>
inline void log::error(const FormatString& format, const Args & ...args)
{
	printf("[%serror%s] ", red, reset);
    fmt::print(fmt::runtime(format), args...);
	printf("\n");
}

template<typename FormatString, typename ...Args>
inline void log::critical(const FormatString& format, const Args & ...args)
{
    printf("[%serror%s] ", red, reset);
    fmt::print(fmt::runtime(format), args...);
    printf("\n");
}

}
