#include "fileio.h"

#include <cassert>
#include <fstream>
#include <map>
#include <unordered_map>

#include "log.h"
#include "tools.h"
#include "resource_pipeline.h"
#include "miniz.h"
#include "json/json.hpp"
#include <filesystem>

namespace fs = std::filesystem;

using namespace std;
using namespace xs;
using namespace xs::fileio;

namespace xs::fileio
{
	map<string, string> wildcards;	
}



void fileio::initialize()
{
	// Get the engine user path [user]
	char* pValue;
	size_t len;
	_dupenv_s(&pValue, &len, "APPDATA");
	if (pValue != nullptr)
	{
		string xs_user_path = string(pValue) + string("\\xs\\");
		if (!fs::exists(xs_user_path))
			fs::create_directory(xs_user_path);
		wildcards["[user]"] = xs_user_path;
	}
	else
	{
		log::critical("Could not get the APPDATA environment variable.");
		assert(false);
	}

	// Load the engine user settings json (if any) and find the game folder
	if (exists("[user]/settings.json"))
	{
		auto settings_str = read_text_file("[user]/settings.json");
		if (!settings_str.empty())
		{
			auto settings = nlohmann::json::parse(settings_str);	
			string game_folder = settings["game"];
			if (!game_folder.empty())
			{
				add_wildcard("[game]", game_folder);
			}
		}
	}
	else
	{
		log::warn("Could not find the user settings.json file.");
		log::info("Loading xs sample and creating settings file.");	
		add_wildcard("[game]", fileio::absolute("samples/hello"));
	}

	// Load the game settings json and find the main game script
	if (exists("[game]/project.json"))
	{
		auto settings_str = read_text_file("[game]/project.json");
		if (!settings_str.empty())
		{
			auto settings = nlohmann::json::parse(settings_str);
			string main_script = settings["main"];
			if(main_script.empty())
				log::error("Could not find the 'main' in the game project.json file.");
		}
		else log::error("Could not read settings.json file.");
	}
	else log::error("Could not find the game project.json file.");
}