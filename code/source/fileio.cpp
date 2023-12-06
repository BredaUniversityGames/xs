#include "fileio.h"
#include <cassert>
#include <fstream>
#include <map>

#include "log.h"
#include "tools.h"

#if defined(PLATFORM_SWITCH)
#include <nn/fs.h>
#include <nn/nn_Assert.h>
#include <nn/nn_Abort.h>
#include <nn/nn_Log.h>
#include <nn/account/account_ApiForApplications.h>
#include "account_switch.h"
#elif defined(PLATFORM_PC)
#include <filesystem>
#endif

#if defined(PLTFORM_APPLE)
#import <Foundation/Foundation.h>
#endif

namespace xs::fileio::internal
{
	std::map<std::string, std::string> wildcards;
}

using namespace xs;
using namespace fileio::internal;
using namespace std;

#if !defined(PLATFORM_APPLE)

void fileio::initialize(/* const std::string& main_script*/)
{
#if defined(PLATFORM_PC)
	add_wildcard("[games]", "./games");
    
#elif defined(PLATFORM_APPLE)
    
#elif defined(PLATFORM_SWITCH)
	nn::Result result;
	size_t cacheSize = 0;

	char* cacheBuffer = nullptr;

	// Mounts the file system.
	// Mounting requires a cache buffer.
	{
		xs::log::info("Mount Rom");

		// Gets the buffer size needed for the file system metadata cache.
		// No error handling is needed. An abort occurs within the library when getting fails.
		(void)nn::fs::QueryMountRomCacheSize(&cacheSize);

		cacheBuffer = new char[cacheSize];
		assert(cacheBuffer);

		// Mounts the file system.
		// Do not release the cache buffer until you unmount.
		result = nn::fs::MountRom("rom", cacheBuffer, cacheSize);

		assert(result.IsSuccess());
	}

	// Mount save data
	{
		log::info("Mount save data");

		// Get the user identifier from the account
		nn::account::Uid user = nn::account::InvalidUid;
		nn::account::GetUserId(&user, xs::account::get_account_handle());

		// Create the selected user save data.
		// If data already exists, does nothing and returns nn::ResultSuccess.
		result = nn::fs::EnsureSaveData(user);
		if (nn::fs::ResultUsableSpaceNotEnough::Includes(result))
		{
			// Error handling when the application does not have enough memory is required for the nn::fs::EnsureSaveData() function.
			// The system automatically displays a message indicating that there was not enough capacity to create save data in the error viewer.
			// The application must offer options to cancel account selection and to return to the prior scene.
			NN_ABORT("Usable space not enough.\n");
		}

		// Mount the save data as "save."
		result = nn::fs::MountSaveData("save", user);
		// Always abort when a failure occurs.
		NN_ABORT_UNLESS_RESULT_SUCCESS(result);
	}

	add_wildcard("[games]", "rom:");
	add_wildcard("[save]", "save:");

#elif defined(PLATFORM_PS5)	
	add_wildcard("[games]", "/app0");
#endif

	// All platforms
	bool success = false;
	if (exists("[games]/.ini"))
	{
		auto game_str = read_text_file("[games]/.ini");
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
	assert(success);

#if defined(PLATFORM_PC)
	if(!success)
	{
		log::info("Please provide a valid game folder in the games/.ini file!");
		log::info("A valid game folder contanins a valid game.wren script.");
		log::info("Check the documentation and the example that was just created.");
		fileio::write_text_file("hello", "[games]/.ini");
		add_wildcard("[game]", "[games]/hello");
	}

	char* pValue;
	size_t len;
	_dupenv_s(&pValue, &len, "APPDATA");
	if (pValue != nullptr)
	{
		auto game_str = read_text_file("[games]/.ini");
		auto tokens = xs::tools::string_split(game_str, "/");
		game_str = tokens[tokens.size() - 1];
		string save_path = string(pValue) + string("\\xs\\");
		if (!filesystem::exists(save_path))
			filesystem::create_directory(save_path);
		save_path.append(game_str);
		if (!filesystem::exists(save_path))
			filesystem::create_directory(save_path);
 		add_wildcard("[save]", save_path);
	}
#elif defined(PLATFORM_PS5)	
	// add_wildcard("[save]", save_path);
#endif
}

#endif

vector<char> fileio::read_binary_file(const std::string& filename)
{
	const auto path = get_path(filename);
	ifstream file(path, std::ios::binary | std::ios::ate);
	if (!file.is_open())
	{
		log::error("File {} with full path {} was not found!", filename, path);
		return vector<char>();	
	}
	const streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);
	std::vector<char> buffer(size);
	if (file.read(buffer.data(), size))
		return  buffer;
	assert(false);
	return vector<char>();
}

string fileio::read_text_file(const string& filename)
{
	const auto path = get_path(filename);	
	ifstream file(path);
	if (!file.is_open())
	{
		log::error("File {} with full path {} was not found!", filename, path);
		return string();
	}
	file.seekg(0, std::ios::end);
	const size_t size = file.tellg();
	string buffer(size, '\0');
	file.seekg(0);
	file.read(&buffer[0], size);
	return buffer;
}

bool xs::fileio::write_text_file(const std::string& text, const std::string& filename)
{
	auto fullpath = fileio::get_path(filename);
	std::ofstream ofs;
	ofs.open(fullpath);
	if (ofs.is_open())
	{
		ofs << text;
		ofs.close();
		return true;
	}
	return false;
}

void fileio::add_wildcard(const string& wildcard, const string& value)
{
	wildcards[wildcard] = value;
}

std::string fileio::get_path(const std::string& filename)
{
	std::string full_path = filename;

	for (const auto& p : wildcards)
	{
		if (full_path.find(p.first) != std::string::npos)
			full_path = tools::string_replace(full_path, p.first, p.second);
	}

	return full_path;
}

bool xs::fileio::exists(const std::string& filename)
{
	const auto path = get_path(filename);
	ifstream f(path.c_str());
	auto good = f.good();
	f.close();
	return good;
}

#if defined(PLATFORM_PC) && (defined(DEBUG) || defined(PROFILE))

uint64_t xs::fileio::last_write(const std::string& filename)
{
	const auto path = get_path(filename);
	std::filesystem::file_time_type ftime = std::filesystem::last_write_time(path);
	return static_cast<uint64_t>(ftime.time_since_epoch().count());
}

#else 

uint64_t xs::fileio::last_write(const std::string& filename) { return 0; }

#endif
