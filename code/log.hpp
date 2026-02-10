#pragma once

#include <cassert>
#include <string>
#include <iostream>
#include "defines.hpp"

// Detect std::format availability
// C++20 std::format is available on:
// - MSVC 2019 16.10+ with /std:c++20 or /std:c++latest
// - GCC 13+ with -std=c++20
// - Clang 14+ with -std=c++20
// - Nintendo Switch SDK (has C++20 support)
#if __has_include(<format>) && defined(__cpp_lib_format)
    #define XS_HAS_STD_FORMAT 1
#else
    #define XS_HAS_STD_FORMAT 0
#endif

// Color support for platforms with ANSI terminal (disabled in Release)
#if !defined(XS_RELEASE) && (defined(PLATFORM_PC) || defined(PLATFORM_LINUX) || defined(PLATFORM_SWITCH) || defined(PLATFORM_PROSPERO))
#define USE_LOG_COLOR
#endif

// UTF-8 support - fancy logging for Debug/Develop (disabled in Release)
#ifndef XS_RELEASE
#define USE_UTF8_LOG
#endif

namespace xs::log
{
#ifdef USE_LOG_COLOR
    constexpr const char* info_color = "\033[38;5;75m";
    constexpr const char* warn_color = "\033[38;5;214m";
    constexpr const char* error_color = "\033[38;5;196m";
    constexpr const char* critical_color = "\033[38;5;196m";
	constexpr const char* reset = "\033[0m";
#else
	constexpr const char* info_color = "";
    constexpr const char* warn_color = "";
    constexpr const char* error_color = "";
    constexpr const char* critical_color = "";
    constexpr const char* reset = "";
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

	template<typename FormatString, typename... Args>
	void script(const FormatString& fmt, const Args&... args);

	void flush();
}

// Implementation: Choose between std::format and fmt library
#if XS_HAS_STD_FORMAT

// Use C++20 std::format
#include <format>

#define XS_FORMAT std::format

#ifdef USE_UTF8_LOG

template<typename FormatString, typename ...Args>
inline void xs::log::info(const FormatString& format, const Args & ...args)
{
	std::string message = std::string("ℹ️  ") + info_color + std::vformat(format, std::make_format_args(args...)) + reset;
	output_log(message);
}

template<typename FormatString, typename ...Args>
inline void xs::log::warn(const FormatString& format, const Args & ...args)
{
	std::string message = std::string("⚠️  ") + warn_color + std::vformat(format, std::make_format_args(args...)) + reset;
	output_log(message);
}

template<typename FormatString, typename ...Args>
inline void xs::log::error(const FormatString& format, const Args & ...args)
{
	std::string message = std::string("⛔️  ") + error_color + std::vformat(format, std::make_format_args(args...)) + reset;
	output_log(message);
}

template<typename FormatString, typename ...Args>
inline void xs::log::critical(const FormatString& format, const Args & ...args)
{
	std::string message = std::string("🚨  ") + critical_color + std::vformat(format, std::make_format_args(args...)) + reset;
	output_log(message);
}

template<typename FormatString, typename ...Args>
inline void xs::log::script(const FormatString& format, const Args & ...args)
{
	std::string message = std::string("📜  ") + std::vformat(format, std::make_format_args(args...));
	output_log(message);
}

#else

template<typename FormatString, typename ...Args>
inline void xs::log::info(const FormatString& format, const Args & ...args)
{
	std::string message = std::format("[{}info{}] ", info_color, reset) + std::vformat(format, std::make_format_args(args...));
	output_log(message);
}

template<typename FormatString, typename ...Args>
inline void xs::log::warn(const FormatString& format, const Args & ...args)
{
	std::string message = std::format("[{}warn{}] ", warn_color, reset) + std::vformat(format, std::make_format_args(args...));
	output_log(message);
}

template<typename FormatString, typename ...Args>
inline void xs::log::error(const FormatString& format, const Args & ...args)
{
	std::string message = std::format("[{}error{}] ", error_color, reset) + std::vformat(format, std::make_format_args(args...));
	output_log(message);
}

template<typename FormatString, typename ...Args>
inline void xs::log::critical(const FormatString& format, const Args & ...args)
{
	std::string message = std::format("[{}error{}] ", error_color, reset) + std::vformat(format, std::make_format_args(args...));
	output_log(message);
}

template<typename FormatString, typename ...Args>
inline void xs::log::script(const FormatString& format, const Args & ...args)
{
	std::string message = std::format("[script] ") + std::vformat(format, std::make_format_args(args...));
	output_log(message);
}

#endif

#else

// Fallback to fmt library
#define FMT_HEADER_ONLY
#include <fmt/core.h>
#define XS_FORMAT fmt::format

#ifdef USE_UTF8_LOG

template<typename FormatString, typename ...Args>
inline void xs::log::info(const FormatString& format, const Args & ...args)
{
	std::string message = std::string("ℹ️  ") + info_color + fmt::format(fmt::runtime(format), args...) + reset;
	output_log(message);
}

template<typename FormatString, typename ...Args>
inline void xs::log::warn(const FormatString& format, const Args & ...args)
{
	std::string message = std::string("⚠️  ") + warn_color + fmt::format(fmt::runtime(format), args...) + reset;
	output_log(message);
}

template<typename FormatString, typename ...Args>
inline void xs::log::error(const FormatString& format, const Args & ...args)
{
	std::string message = std::string("⛔️  ") + error_color + fmt::format(fmt::runtime(format), args...) + reset;
	output_log(message);
}

template<typename FormatString, typename ...Args>
inline void xs::log::critical(const FormatString& format, const Args & ...args)
{
	std::string message = std::string("🚨  ") + critical_color + fmt::format(fmt::runtime(format), args...) + reset;
	output_log(message);
}

template<typename FormatString, typename ...Args>
inline void xs::log::script(const FormatString& format, const Args & ...args)
{
	std::string message = std::string("📜  ") + fmt::format(fmt::runtime(format), args...);
	output_log(message);
}

#else

template<typename FormatString, typename ...Args>
inline void xs::log::info(const FormatString& format, const Args & ...args)
{
	std::string message = fmt::format("[{}info{}] ", info_color, reset) + fmt::format(fmt::runtime(format), args...);
	output_log(message);
}

template<typename FormatString, typename ...Args>
inline void xs::log::warn(const FormatString& format, const Args & ...args)
{
	std::string message = fmt::format("[{}warn{}] ", warn_color, reset) + fmt::format(fmt::runtime(format), args...);
	output_log(message);
}

template<typename FormatString, typename ...Args>
inline void xs::log::error(const FormatString& format, const Args & ...args)
{
	std::string message = fmt::format("[{}error{}] ", error_color, reset) + fmt::format(fmt::runtime(format), args...);
	output_log(message);
}

template<typename FormatString, typename ...Args>
inline void xs::log::critical(const FormatString& format, const Args & ...args)
{
	std::string message = fmt::format("[{}error{}] ", error_color, reset) + fmt::format(fmt::runtime(format), args...);
	output_log(message);
}

template<typename FormatString, typename ...Args>
inline void xs::log::script(const FormatString& format, const Args & ...args)
{
	std::string message = fmt::format("[script] ") + fmt::format(fmt::runtime(format), args...);
	output_log(message);
}

#endif 

#endif

inline void xs::log::flush()
{
	std::cout.flush();
}
