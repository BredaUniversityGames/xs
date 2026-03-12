#include "fileio.hpp"

#include <cassert>
#include <fstream>
#include <map>
#include <unordered_map>

#include "log.hpp"
#include "tools.hpp"
#include "miniz.h"
#include "json/json.hpp"
#include "xs.hpp"
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#endif

#define PUBLISH 0

namespace fs = std::filesystem;

using namespace std;
using namespace xs;
using namespace xs::fileio;

namespace xs::fileio::internal
{
	extern map<string, string> wildcards;	
}

void fileio::initialize(const std::string& game_path)
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

	// Determine game folder path
	// Priority: CLI argument > settings.json > default sample
	bool success = false;

	// 1. Use CLI-provided game path if available
	if (!game_path.empty())
	{
		if (xs::get_run_mode() == xs::run_mode::development ||
			xs::get_run_mode() == xs::run_mode::packaging)
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

	// 3. Fallback to default sample
	if (!success)
	{
		log::warn("Could not find the user settings.json file.");
		log::info("Loading xs sample and creating settings file.");
		string xs_sample = fileio::absolute("samples/hello");
		add_wildcard("[game]", xs_sample);
	}

#if PUBLISH
	// TODO: This needs to be double-checked for packaged and publlished builds
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

	// Set the shared resources folder - this is where the engine resources are stored
	if(xs::get_run_mode() != xs::run_mode::packaged)
	{
		std::string local_resources = "resources";  // Relative to pwd (development)
		
		// Get executable directory for installed engine path
		std::string exe_dir;
#ifdef _WIN32
		char buffer[MAX_PATH];
		GetModuleFileNameA(NULL, buffer, MAX_PATH);
		std::string exe_path(buffer);
		exe_dir = exe_path.substr(0, exe_path.find_last_of("\\/"));
#else
		// On Linux/Unix, use /proc/self/exe
		char buffer[PATH_MAX];
		ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
		if (len != -1)
		{
			buffer[len] = '\0';
			std::string exe_path(buffer);
			exe_dir = exe_path.substr(0, exe_path.find_last_of("/"));
		}
#endif
		
		std::string installed_resources = exe_dir + "/resources";
		
		// Priority: Check pwd-relative first (development), then exe-relative (installed)
		if (fs::exists(local_resources))
		{
			add_wildcard("[shared]", local_resources);
			log::info("Using local resources: {}", fs::absolute(local_resources).string());
		}
		else if (fs::exists(installed_resources))
		{
			add_wildcard("[shared]", installed_resources);
			log::info("Using installed engine resources: {}", installed_resources);
		}
		else
		{
			log::warn("Could not find shared resources folder! Tried: {} and {}", 
				fs::absolute(local_resources).string(), installed_resources);
			add_wildcard("[shared]", local_resources);  // Set anyway as fallback
		}
	}
}