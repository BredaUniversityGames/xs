#include "fileio.hpp"
#include "log.hpp"
#include "xs.hpp"
#include <string>
#import <filesystem>
#import <Foundation/Foundation.h>

using namespace xs;
using namespace std;

namespace
{
    string get_bundle_resources_path()
    {
        NSBundle* bundle = [NSBundle mainBundle];
        NSString* resourcePath = [bundle resourcePath];
        return string([resourcePath UTF8String]);
    }
}

string fileio::internal::get_user_path()
{
    NSString* user_path_oc = @"~/Library/Application Support/xs";
    return string([user_path_oc.stringByExpandingTildeInPath UTF8String]);
}

string fileio::internal::get_shared_path()
{
    if (xs::get_run_mode() == xs::run_mode::packaged)
        return {};

    string bundled_resources = get_bundle_resources_path() + "/resources";
    if (filesystem::exists(bundled_resources))
    {
        log::info("Shared resources (from bundle): {}", bundled_resources);
        return bundled_resources;
    }

    log::info("Shared resources (relative): resources");
    return "resources";
}

string fileio::internal::resolve_package_path(const string& game_path)
{
    // Try absolute path first, then bundle-relative
    if (fileio::exists(game_path))
        return game_path;

    return get_bundle_resources_path() + "/" + game_path;
}
