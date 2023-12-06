#include "log.h"
#include "../include/version.h"


void xs::log::initialize()
{
#ifdef PLATFORM_APPLE
    log::info("  __ __ _____  ");
    log::info(" |  |  |   __| ");
    log::info(" |-   -|__   | ");
    log::info(" |__|__|_____| " + xs::version::version_string);
    log::info("Made with love at Breda University of Applied Sciences");
    log::info("");
#else
    log::info("\033[33m  __ __ _____  \033[0m");
    log::info("\033[33m |  |  |   __| \033[0m");
    log::info("\033[33m |-   -|__   | \033[0m");
    log::info("\033[33m |__|__|_____| \033[0m" + xs::version::version_string);
    log::info("Made with love at Breda University of Applied Sciences");
    log::info("");
#endif
}

