#include "fileio.hpp"

#include <cassert>
#include <fstream>
#include <map>
#include <unordered_map>

#include "log.hpp"
#include "tools.hpp"
#include "resource_pipeline.hpp"
#include "miniz.h"
#include "json/json.hpp"
#include <filesystem>

#define PUBLISH 0

namespace fs = std::filesystem;

using namespace std;
using namespace xs;
using namespace xs::fileio;

namespace xs::fileio::internal
{
	extern map<string, string> wildcards;	
}

void fileio::initialize()
{
	// Get the engine user path [user]
	char* pValue;
	size_t len;
	_dupenv_s(&pValue, &len, "APPDATA");
	if (pValue != nullptr)
	{
		string xs_user_path = string(pValue) + string("\\xs");
		if (!fs::exists(xs_user_path))
			fs::create_directory(xs_user_path);
		internal::wildcards["[user]"] = xs_user_path;		
	}
	else
	{
		log::critical("Could not get the APPDATA environment variable.");
		assert(false);
	}

	// The save location for the game
	auto xs_user_path = internal::wildcards["[user]"];
	string game_save_path = xs_user_path + "\\save";
	if (!fs::exists(game_save_path))
		fs::create_directory(game_save_path);
	internal::wildcards["[save]"] = game_save_path;

	// Load the engine user settings json (if any) and find the game folder
	bool success = false;
	if (exists("[user]/settings.json"))
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
					log::info("Game folder {} ", game_folder);
					success = true;
				}
			}					
		}
		else
		{
			log::warn("Could not read the user settings.json file.");
		}
	}
	if (!success)
	{
		log::warn("Could not find the user settings.json file.");
		log::info("Loading xs sample and creating settings file.");
		string xs_sample = fileio::absolute("samples/hello");
		add_wildcard("[game]", xs_sample);
	}

#if PUBLISH
	add_wildcard("[game]", "game");
#endif

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

			//if (settings.find("Main") == settings.end())
			//	log::error("Could not find the 'main' in the game project.json file.");
		}
		else log::error("Could not read project.json file.");
	}
	else log::error("Could not find the game project.json file.");

	// Set the shared assets folder - this is where the shared assets are stored
	add_wildcard("[shared]", "assets");
}