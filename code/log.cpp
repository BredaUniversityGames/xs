#include "log.hpp"
#include "version.hpp"
#include "xs.hpp"

#if defined(PLATFORM_PC)
#include <windows.h>
#endif

// UTF-8 - Modern terminals with emoji and unicode
#ifdef USE_UTF8_LOG

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

    // Modern UTF-8 version with rounded borders and gradient colors
    printf("\n");
    printf("\033[38;5;208m╭──────────────────────────────────────────────────────────────────────────────────────────────────╮\033[0m\n");
    printf("\033[38;5;208m│\033[0m                                                                                                  \033[38;5;208m│\033[0m\n");
    printf("\033[38;5;208m│\033[0m  \033[38;5;208m▀▄▀ █▀▀\033[0m   v-%s:%-65s  \033[38;5;208m│\033[0m\n", xs::version::version_string.c_str(), ::get_mode_name().c_str());
    printf("\033[38;5;208m│\033[0m  \033[38;5;208m█ █ ▄▄█\033[0m   🧡 Breda University of Applied Sciences                                               \033[38;5;208m│\033[0m\n");
    printf("\033[38;5;208m│\033[0m                                                                                                  \033[38;5;208m│\033[0m\n");
    printf("\033[38;5;208m╰──────────────────────────────────────────────────────────────────────────────────────────────────╯\033[0m\n");
}

// Basic ASCII with color codes
#else

void xs::log::initialize()
{
    printf("\n");
    printf(" %s  __ __ _____  %s   xs game engine %s\n", yellow, reset, xs::version::version_string.c_str());
    printf(" %s |  |  |   __| %s\n", yellow, reset);
    printf(" %s |-   -|__   | %s   Built with care at Breda University of Applied Sciences\n", yellow, reset);
    printf(" %s |__|__|_____| %s\n", yellow, reset);
    printf("\n");
}

#endif

