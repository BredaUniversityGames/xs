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
    constexpr std::string info_color = "\033[38;5;75m";
    constexpr std::string warn_color = "\033[38;5;214m";
    constexpr std::string error_color = " \033[38;5;196m";
    constexpr std::string critical_color = " \033[38;5;196m";
	constexpr auto reset = "\033[0m";
#else
	constexpr std::string info_color = "";
    constexpr std::string warn_color = "";
    constexpr std::string error_color = "";
    constexpr std::string critical_color = "";
    constexpr std::string reset = "\n";
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
	std::string message = "ℹ️ " + info_color + fmt::format(fmt::runtime(format), args...) + reset;
	output_log(message);
}

template<typename FormatString, typename ...Args>
inline void xs::log::warn(const FormatString& format, const Args & ...args)
{
	std::string message = "⚠️ " + warn_color + fmt::format(fmt::runtime(format), args...) + reset;
	output_log(message);
}

template<typename FormatString, typename ...Args>
inline void xs::log::error(const FormatString& format, const Args & ...args)
{
	std::string message = "⛔️ " + error_color + fmt::format(fmt::runtime(format), args...) + reset;
	output_log(message);
}

template<typename FormatString, typename ...Args>
inline void xs::log::critical(const FormatString& format, const Args & ...args)
{
	std::string message = "🚨 " + critical_color + fmt::format(fmt::runtime(format), args...) + reset;
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
