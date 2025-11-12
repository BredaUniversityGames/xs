#pragma once

#include <cassert>
#define FMT_HEADER_ONLY
#include <fmt/core.h>

// Color support for platforms with ANSI terminal
#if defined(PLATFORM_PC) || defined(PLATFORM_SWITCH) || defined(PLATFORM_PROSPERO)
#define USE_LOG_COLOR
#endif

// UTF-8 support - everyone gets the modern experience!
#define USE_UTF8_LOG

namespace xs::log
{
#ifdef USE_LOG_COLOR
	constexpr auto green = "\033[32m";
	constexpr auto magenta = "\033[35m";
	constexpr auto red = "\033[31m";
	constexpr auto yellow = "\033[33m";
	constexpr auto reset = "\033[0m";
#else
	constexpr auto green = "";
	constexpr auto magenta = "";
	constexpr auto red = "";
	constexpr auto yellow = "";
	constexpr auto reset = "";
#endif

	void initialize();

	template<typename FormatString, typename... Args>
	void info(const FormatString& fmt, const Args&... args);

	template<typename FormatString, typename... Args>
	void warn(const FormatString& fmt, const Args&... args);

	template<typename FormatString, typename... Args>
	void error(const FormatString& fmt, const Args&... args);

	template<typename FormatString, typename... Args>
	void critical(const FormatString& fmt, const Args&... args);

	void flush();
}

#ifdef USE_UTF8_LOG

template<typename FormatString, typename ...Args>
inline void xs::log::info(const FormatString& format, const Args & ...args)
{
	printf("ℹ️  \033[38;5;75m");  // Blue (VS Code info color)
	fmt::print(fmt::runtime(format), args...);
	printf("\033[0m\n");
}

template<typename FormatString, typename ...Args>
inline void xs::log::warn(const FormatString& format, const Args & ...args)
{
	printf("⚠️  \033[38;5;214m");  // Orange (VS Code warning color)
	fmt::print(fmt::runtime(format), args...);
	printf("\033[0m\n");
}

template<typename FormatString, typename ...Args>
inline void xs::log::error(const FormatString& format, const Args & ...args)
{
	printf("⛔️ \033[38;5;196m");  // Red (VS Code error color)
	fmt::print(fmt::runtime(format), args...);
	printf("\033[0m\n");
}

template<typename FormatString, typename ...Args>
inline void xs::log::critical(const FormatString& format, const Args & ...args)
{
	printf("🚨 \033[38;5;196m");  // Bright red (VS Code error color)
	fmt::print(fmt::runtime(format), args...);
	printf("\033[0m\n");
}

#else

template<typename FormatString, typename ...Args>
inline void xs::log::info(const FormatString& format, const Args & ...args)
{
	printf("[%sinfo%s] ", green, reset);
	fmt::print(fmt::runtime(format), args...);
	printf("\n");
}

template<typename FormatString, typename ...Args>
inline void xs::log::warn(const FormatString& format, const Args & ...args)
{
	printf("[%swarn%s] ", magenta, reset);
	fmt::print(fmt::runtime(format), args...);
	printf("\n");
}

template<typename FormatString, typename ...Args>
inline void xs::log::error(const FormatString& format, const Args & ...args)
{
	printf("[%serror%s] ", red, reset);
	fmt::print(fmt::runtime(format), args...);
	printf("\n");
}

template<typename FormatString, typename ...Args>
inline void xs::log::critical(const FormatString& format, const Args & ...args)
{
	printf("[%serror%s] ", red, reset);
	fmt::print(fmt::runtime(format), args...);
	printf("\n");
}

#endif

inline void xs::log::flush()
{
	fflush(stdout);
}
