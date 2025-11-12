#include "fileio.hpp"
#include "log.hpp"
#include "json/json.hpp"
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
    NSArray* docs_paths_oc = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString* docs_path_oc  = [docs_paths_oc objectAtIndex:0];

    // NSArray* user_paths_oc = NSSearchPathForDirectoriesInDomains(NSUserDirectory, NSUserDomainMask, YES);
    // NSString* user_path_oc = [user_paths_oc objectAtIndex:0];

#if defined(XS_DEBUG) && defined(PLATFORM_MAC)
    NSString* project_path_oc = PROJECT_DIR;  // Define from a macro
#else
    NSString* project_path_oc = [[NSBundle mainBundle] resourcePath];
#endif

    // Shared
    string project_path = [project_path_oc UTF8String];
    auto shared_path = project_path + "/assets";
    fileio::internal::wildcards["[shared]"] = shared_path;

    // Save
    string docs_path_c = [docs_path_oc UTF8String];
    string docs_path = string(docs_path_c);
    fileio::internal::wildcards["[save]"] = docs_path;

    // User
    NSString* user_path_oc = @"~/Library/Preferences/xs";
    string user_path = [user_path_oc.stringByExpandingTildeInPath UTF8String];

    if(!filesystem::exists(user_path))
        filesystem::create_directory(user_path);
    fileio::internal::wildcards["[user]"] = [@"~/Library/Preferences/xs".stringByExpandingTildeInPath UTF8String];

    // Determine game folder path
    // Priority: CLI argument > settings.json > default sample
    bool success = false;

    // 1. Use CLI-provided game path if available
    if (!game_path.empty())
    {
        string resolved_path = fileio::absolute(game_path);
        add_wildcard("[game]", resolved_path);
        log::info("Game folder (from CLI): {} ", resolved_path);
        success = true;
    }
    // 2. Load from engine user settings json (if any)
    else if (exists("[user]/settings.json"))
    {
        auto settings_str = read_text_file("[user]/settings.json");
        if (!settings_str.empty())
        {
            auto settings = nlohmann::json::parse(settings_str);
            auto game_json = settings["game"];
            auto type_json = game_json["type"];
            auto value_json = game_json["value"];

            if (type_json.is_string() && value_json.is_string())
            {
                string game_folder = value_json.get<string>();
                if (!game_folder.empty())
                {
                    add_wildcard("[game]", game_folder);
                    log::info("Game folder (from settings): {} ", game_folder);
                    success = true;
                }
            }
        }
        else
        {
            log::warn("Could not read the user settings.json file.");
        }
    }

    // 3. Fallback to default sample
    if (!success)
    {
        log::warn("Could not find the user settings.json file.");
        log::info("Loading xs sample and creating settings file.");
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
                string main = settings["Main"]["value"];
                add_wildcard("[main]", main);
            }
            else log::error("Could not find the 'main' in the game project.json file.");
        }
        else log::error("Could not read project.json file.");
    }
    else log::error("Could not find the game project.json file.");

    // Set the shared assets folder - this is where the shared assets are stored
    add_wildcard("[shared]", "assets");
}
