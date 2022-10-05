#include "log.h"
#include "version.h"

#if defined(PLATFORM_PC) || defined(PLATFORM_SWITCH)

#include <spdlog/cfg/env.h>

void xs::log::initialize()
{
    spdlog::cfg::load_env_levels();

#ifdef PLATFORM_PC
    spdlog::set_pattern("[%Y-%m-%d %T.%e] [%^%l%$] %v");
#elif PLATFORM_SWITCH
    spdlog::set_pattern("[%^%l%$] %v");
#endif    

#if 0 // Samples from spdlog for testing
    spdlog::info("Spdlog Version {}.{}.{}", SPDLOG_VER_MAJOR, SPDLOG_VER_MINOR, SPDLOG_VER_PATCH);
    spdlog::warn("Easy padding in numbers like {:08d}", 12);
    spdlog::critical("Support for int: {0:d};  hex: {0:x};  oct: {0:o}; bin: {0:b}", 42);
    spdlog::info("Support for floats {:03.2f}", 1.23456);
    spdlog::info("Positional args are {1} {0}..", "too", "supported");
    spdlog::info("{:>8} aligned, {:<8} aligned", "right", "left");
    spdlog::error("ERR!?");
#endif

    log::info("\033[33m  __ __ _____  \033[0m");
    log::info("\033[33m |  |  |   __| \033[0m");
    log::info("\033[33m |-   -|__   | \033[0m");
    log::info("\033[33m |__|__|_____| \033[0m" + xs::version::version_string);
    log::info("Made with love at Breda University of Applied Sciences");
    log::info("");
}

#elif defined(PLATFORM_PS5)

void xs::log::initialize() {}

#endif

