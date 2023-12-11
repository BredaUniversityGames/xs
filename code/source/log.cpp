#include "log.h"
#include "../include/version.h"


void xs::log::initialize()
{
    log::info("{}  __ __ _____  {}", yellow, reset);
    log::info("{} |  |  |   __| {}", yellow, reset);
    log::info("{} |-   -|__   | {}", yellow, reset);
    log::info("{} |__|__|_____| {}" + xs::version::version_string, yellow, reset);
    log::info("Made with love at Breda University of Applied Sciences");
}

