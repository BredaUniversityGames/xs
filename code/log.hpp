#pragma once

#include <cassert>
#include <string>
#include <iostream>
#define FMT_HEADER_ONLY
#include <fmt/core.h>

// Color support for platforms with ANSI terminal (disabled in Release)
#if !defined(XS_RELEASE) && (defined(PLATFORM_PC) || defined(PLATFORM_SWITCH) || defined(PLATFORM_PROSPERO))
#define USE_LOG_COLOR
#endif

// UTF-8 support - fancy logging for Debug/Develop (disabled in Release)
#ifndef XS_RELEASE
#define USE_UTF8_LOG
#endif

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

	// Internal helper for logging to appropriate output (console or debugger)
	void output_log(const std::string& message);

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
	std::string message = "ℹ️  \033[38;5;75m" + fmt::format(fmt::runtime(format), args...) + "\033[0m\n";
	output_log(message);
}

template<typename FormatString, typename ...Args>
inline void xs::log::warn(const FormatString& format, const Args & ...args)
{
	std::string message = "⚠️  \033[38;5;214m" + fmt::format(fmt::runtime(format), args...) + "\033[0m\n";
	output_log(message);
}

template<typename FormatString, typename ...Args>
inline void xs::log::error(const FormatString& format, const Args & ...args)
{
	std::string message = "⛔️ \033[38;5;196m" + fmt::format(fmt::runtime(format), args...) + "\033[0m\n";
	output_log(message);
}

template<typename FormatString, typename ...Args>
inline void xs::log::critical(const FormatString& format, const Args & ...args)
{
	std::string message = "🚨 \033[38;5;196m" + fmt::format(fmt::runtime(format), args...) + "\033[0m\n";
	output_log(message);
}

#else

template<typename FormatString, typename ...Args>
inline void xs::log::info(const FormatString& format, const Args & ...args)
{
	std::string message = fmt::format("[{}info{}] ", green, reset) + fmt::format(fmt::runtime(format), args...) + "\n";
	output_log(message);
}

template<typename FormatString, typename ...Args>
inline void xs::log::warn(const FormatString& format, const Args & ...args)
{
	std::string message = fmt::format("[{}warn{}] ", magenta, reset) + fmt::format(fmt::runtime(format), args...) + "\n";
	output_log(message);
}

template<typename FormatString, typename ...Args>
inline void xs::log::error(const FormatString& format, const Args & ...args)
{
	std::string message = fmt::format("[{}error{}] ", red, reset) + fmt::format(fmt::runtime(format), args...) + "\n";
	output_log(message);
}

template<typename FormatString, typename ...Args>
inline void xs::log::critical(const FormatString& format, const Args & ...args)
{
	std::string message = fmt::format("[{}error{}] ", red, reset) + fmt::format(fmt::runtime(format), args...) + "\n";
	output_log(message);
}

#endif

inline void xs::log::flush()
{
	std::cout.flush();
}
