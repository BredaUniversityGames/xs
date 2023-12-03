//
//  fileio_apple.m
//  xs
//
//  Created by Bojan Endrovski on 23/08/2023.
//

#import <Foundation/Foundation.h>

#include "fileio.h"
#include <map>
#include <string>

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
    
#if defined(DEBUG) && defined(PLATFORM_MAC)
    NSString* games_path_oc = PROJECT_DIR;  // Define from a macro
#else
    NSString* games_path_oc = [[NSBundle mainBundle] resourcePath];
#endif
        
    auto games_path_c = [games_path_oc UTF8String];        
    string games_path = string(games_path_c) + "/games";
    
    auto docs_path_c = [docs_path_oc UTF8String];
    string docs_path = string(docs_path_c);
    
    xs::fileio::internal::wildcards["[games]"] = games_path;
    xs::fileio::internal::wildcards["[save]"] = docs_path;
    
    // All platforms
    bool success = false;
    if (exists("[games]/.ini"))
    {
        auto game_str = read_text_file("[games]/.ini");
        game_str.erase(std::remove(game_str.begin(), game_str.end(), '\n'), game_str.cend());
        if (!game_str.empty())
        {
            string cwd = "[games]/" + game_str;
            if (exists(cwd + "/game.wren"))
            {
                cwd = get_path(cwd);
                add_wildcard("[game]", cwd);
                success = true;
            }
        }
    }
    
}
