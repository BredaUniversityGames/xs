#include "log.hpp"
#include "version.hpp"
#include "xs.hpp"
#include <sstream>

#if defined(PLATFORM_PC)
#include <windows.h>
#endif

// Common output function used by all logging configurations
void xs::log::output_log(const std::string& message)
{
    // Always output to console (if available)
    std::cout << message;

    #if defined(PLATFORM_PC) && defined(XS_RELEASE)
    // In Release builds without console, also output to debugger
    OutputDebugStringA(message.c_str());
    #endif
}

// UTF-8 - Modern terminals with emoji and unicode
#ifdef USE_UTF8_LOG

using namespace std;

namespace
{
    std::string get_mode_name()
    {
		auto mode = xs::get_run_mode();
        switch (mode)
        {
        case xs::run_mode::development:
            return "development";
        case xs::run_mode::packaged:
            return "packaged";
        case xs::run_mode::packaging:
            return "packaging";
        default:
            return "unknown";
        }
    }
}

void xs::log::initialize()
{
    // Enable UTF-8 output on Windows
    #if defined(PLATFORM_PC)
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    #endif

    std::ostringstream banner;
#ifdef USE_LOG_COLOR
    // Modern UTF-8 version with rounded borders and gradient colors
    banner << "\n";
    banner << "\033[38;5;208m╭──────────────────────────────────────────────────────────────────────────────────────────────────╮\033[0m\n";
    banner << "\033[38;5;208m│\033[0m                                                                                                  \033[38;5;208m│\033[0m\n";
    banner << fmt::format("\033[38;5;208m│\033[0m  \033[38;5;208m▀▄▀ █▀▀\033[0m   version: {:<75}  \033[38;5;208m│\033[0m\n",
        xs::version::get_version_string(false, true, true));
    banner << "\033[38;5;208m│\033[0m  \033[38;5;208m█ █ ▄▄█\033[0m   🧡 Breda University of Applied Sciences                                               \033[38;5;208m│\033[0m\n";
    banner << "\033[38;5;208m│\033[0m                                                                                                  \033[38;5;208m│\033[0m\n";
    banner << "\033[38;5;208m╰──────────────────────────────────────────────────────────────────────────────────────────────────╯\033[0m\n";
#else
    // UTF-8 version without colors
    banner << "\n";
    banner << "╭──────────────────────────────────────────────────────────────────────────────────────────────────╮\n";
    banner << "│                                                                                                  │\n";
    banner << fmt::format("│  ▀▄▀ █▀▀   version: {:<75}  │\n",
        xs::version::get_version_string(false, true, true));
    banner << "│  █ █ ▄▄█   🧡 Breda University of Applied Sciences                                               │\n";
    banner << "│                                                                                                  │\n";
    banner << "╰──────────────────────────────────────────────────────────────────────────────────────────────────╯\n";
#endif

    output_log(banner.str());
}

// Basic ASCII with color codes (or plain ASCII in Release)
#else

void xs::log::initialize()
{
    std::ostringstream banner;
    banner << "\n";
    banner << fmt::format(" {}  __ __ _____  {}   xs game engine {}\n", warn_color, reset, xs::version::get_version_string().c_str());
    banner << fmt::format(" {} |  |  |   __| {}\n", warn_color, reset);
    banner << fmt::format(" {} |-   -|__   | {}   Crafted at Breda University of Applied Sciences\n", warn_color, reset);
    banner << fmt::format(" {} |__|__|_____| {}\n", warn_color, reset);
    banner << "\n";

    output_log(banner.str());
}

#endif

