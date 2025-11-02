#include "version.hpp"
#include <sstream>

namespace xs::version
{
    std::string get_version_string(bool include_hash, bool include_config, bool include_platform)
    {
        std::ostringstream version;

        // Start with base version: YY.BuildNumber
        version << XS_VERSION_YEAR << "." << XS_VERSION_BUILD;

        // Add commit hash if requested
        if (include_hash)
            version << "+" << XS_COMMIT_HASH;

        // Add build configuration
        if (include_config)
        {
            #if defined(DEBUG) || defined(_DEBUG)
                version << "[dbg]";
            #elif defined(NDEBUG)
                version << "[rel]";
            #else
                version << "[dev]";
            #endif
        }

        // Add platform
        if (include_platform)
        {
            #if defined(PLATFORM_PC)
                version << "[pc]";
            #elif defined(PLATFORM_SWITCH)
                version << "[switch]";
            #elif defined(PLATFORM_PS5)
                version << "[ps5]";
            #elif defined(PLATFORM_MAC) || defined(__APPLE__)
                version << "[mac]";
            #elif defined(__linux__)
                version << "[linux]";
            #else
                version << "[unknown]";
            #endif
        }

        return version.str();
    }
}
