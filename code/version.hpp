#pragma once
#include <string>

namespace xs::version
{
    // Base version: YY.BuildNumber
    constexpr const char* XS_VERSION = "25.120";

    // Full version with commit hash: YY.BuildNumber+hash
    constexpr const char* XS_VERSION_FULL = "25.120+09df1d3";

    // Short commit hash
    constexpr const char* XS_COMMIT_HASH = "09df1d3";

    // Version string builder function (implemented in version.cpp)
    // Parameters:
    //   include_hash: Include commit hash (default: true)
    //   include_config: Include build configuration like [dbg], [dev], [rel] (default: true)
    //   include_platform: Include platform like [pc], [switch], [ps5] (default: true)
    std::string get_version_string(bool include_hash = true, bool include_config = true, bool include_platform = true);
}
