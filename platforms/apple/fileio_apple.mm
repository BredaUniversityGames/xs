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
}

void fileio::initialize(const std::string& game_path)
{
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
    // Priority: CLI argument > default sample
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
            success = fileio::load_package(game_path);
        }
    }

    // 2. Fallback to default sample
    if (!success)
    {
        log::info("No game path provided, loading xs sample.");
        string xs_sample = fileio::absolute("samples/hello");
        add_wildcard("[game]", xs_sample);
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
    if (xs::get_run_mode() != xs::run_mode::packaged)
        add_wildcard("[shared]", "assets");
}
