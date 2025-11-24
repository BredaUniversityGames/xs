#include "fileio.hpp"
#include "log.hpp"
#include "json/json.hpp"
#include "xs.hpp"
#include <map>
#include <string>
#import <filesystem>
#import <Foundation/Foundation.h>

using namespace xs;
using namespace std;

namespace xs::fileio::internal
{
    extern std::map<std::string, std::string> wildcards;

    static std::string get_bundle_resources_path()
    {
        NSBundle* bundle = [NSBundle mainBundle];
        NSString* resourcePath = [bundle resourcePath];
        return std::string([resourcePath UTF8String]);
    }
}

void fileio::initialize(const std::string& game_path)
{
    // Get bundle resources path for resolving bundled assets
    string bundle_resources = internal::get_bundle_resources_path();
    log::info("Bundle resources path: {}", bundle_resources);

    // Get the engine user path [user] - ~/Library/Application Support/xs
    NSString* user_path_oc = @"~/Library/Application Support/xs";
    string user_path = [user_path_oc.stringByExpandingTildeInPath UTF8String];
    if (!filesystem::exists(user_path))
        filesystem::create_directory(user_path);
    internal::wildcards["[user]"] = user_path;

    // The save location for the game
    string game_save_path = user_path + "/save";
    if (!filesystem::exists(game_save_path))
        filesystem::create_directory(game_save_path);
    internal::wildcards["[save]"] = game_save_path;

    // Determine game folder path
    // Priority: CLI argument > bundle resources > default sample
    bool success = false;

    // 1. Use CLI-provided game path if available
    if (!game_path.empty())
    {
        if (xs::get_run_mode() == xs::run_mode::development)
        {
            string resolved_path = fileio::absolute(game_path);
            add_wildcard("[game]", resolved_path);
            log::info("Game folder (from CLI): {} ", resolved_path);
            success = true;
        }
        else if (xs::get_run_mode() == xs::run_mode::packaged)
        {
            // First try absolute path, then try in bundle resources
            string package_path = game_path;
            if (!fileio::exists(package_path))
            {
                package_path = bundle_resources + "/" + game_path;
            }
            success = fileio::load_package(package_path);
        }
    }

    // 2. Fallback: check for game in bundle resources
    if (!success)
    {
        string bundled_game = bundle_resources + "/game";
        if (filesystem::exists(bundled_game))
        {
            add_wildcard("[game]", bundled_game);
            log::info("Game folder (from bundle): {}", bundled_game);
            success = true;
        }
    }

    // Load the game settings json and find the main game script
    if (exists("[game]/project.json"))
    {
        auto settings_str = read_text_file("[game]/project.json");
        if (!settings_str.empty())
        {
            auto settings = nlohmann::json::parse(settings_str);
            if (settings.find("Main") != settings.end())
            {
                auto main = settings["Main"]["value"];
                if (main.is_string())
                    add_wildcard("[main]", main.get<string>());
            }
            else log::error("Could not find the 'main' in the game project.json file.");
        }
        else log::error("Could not read project.json file.");
    }
    else log::error("Could not find the game project.json file.");

    // Set the shared assets folder - this is where the engine assets are stored
    // First check bundle resources, then fall back to relative path (for development)
    if (xs::get_run_mode() != xs::run_mode::packaged)
    {
        string bundled_assets = bundle_resources + "/assets";
        if (filesystem::exists(bundled_assets))
        {
            add_wildcard("[shared]", bundled_assets);
            log::info("Shared assets (from bundle): {}", bundled_assets);
        }
        else
        {
            add_wildcard("[shared]", "assets");
            log::info("Shared assets (relative): assets");
        }
    }
}
