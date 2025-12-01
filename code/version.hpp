#pragma once
#include <string>

namespace xs::version
{
    // Version components (single source of truth)
    constexpr int XS_VERSION_YEAR = 25;
    constexpr int XS_VERSION_BUILD = 197;

    // Short commit hash
    constexpr const char* XS_COMMIT_HASH = "5e0f648";

    // Version string builder function (implemented in version.cpp)
    // Builds version strings from the integer components above
    // Parameters:
    //   include_hash: Include commit hash (default: false)
    //   include_config: Include build configuration like [dbg], [dev], [rel] (default: false)
    //   include_platform: Include platform like [pc], [switch], [ps5] (default: false)
    std::string get_version_string(bool include_hash = false, bool include_config = false, bool include_platform = false);
}
