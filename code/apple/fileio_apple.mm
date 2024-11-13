#include "fileio.h"
#include "log.h"
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

void fileio::initialize()
{
    NSArray* docs_paths_oc = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString* docs_path_oc  = [docs_paths_oc objectAtIndex:0];
    
    // NSArray* user_paths_oc = NSSearchPathForDirectoriesInDomains(NSUserDirectory, NSUserDomainMask, YES);
    // NSString* user_path_oc = [user_paths_oc objectAtIndex:0];
    
#if defined(DEBUG) && defined(PLATFORM_MAC)
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
    
    // Load the engine user settings json (if any) and find the game folder
    if (exists("[user]/settings.json"))
    {
        auto settings_str = read_text_file("[user]/settings.json");
        if (!settings_str.empty())
        {
            auto settings = nlohmann::json::parse(settings_str);
            auto game_json = settings["game"];
            auto type_json = game_json["type"];
            assert(type_json.is_string());
            auto value_json = game_json["value"];
            assert(value_json.is_string());
            string game_folder = value_json.get<string>();
            if (!game_folder.empty())
            {
                add_wildcard("[game]", game_folder);
                log::info("Game folder {} ", game_folder);
            }
        }
        else
        {
            log::warn("Could not read the user settings.json file.");
        }
    }
    else
    {
        log::warn("Could not find the user settings.json file.");
        log::info("Loading xs sample and creating settings file.");
        string xs_sample = fileio::absolute("samples/hello");
        add_wildcard("[game]", xs_sample);
    }
}
