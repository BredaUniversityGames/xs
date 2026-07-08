#include "fileio.hpp"

#include <cassert>
#include <fstream>
#include <map>
#include <unordered_map>

#include "log.hpp"
#include "tools.hpp"
#include "packager.hpp"
#include "xs.hpp"
#include "miniz.h"
#include "json/json.hpp"

#if defined(PLATFORM_PC) || defined(PLATFORM_MAC)
#include <filesystem>
#endif

namespace fs = std::filesystem;

using namespace std;

namespace xs::fileio::internal
{
	map<string, string> wildcards;
	// Package data - loaded once on startup (will be populated later)
	static packager::package loaded_package;
	static unordered_map<std::string, const packager::package_entry*> content_map;
}

using namespace xs;
using namespace fileio::internal;

// Load game package
bool xs::fileio::load_package(const std::string& package_path)
{
	if (!fileio::exists(package_path))
	{
		log::info("Game package not found: {}", package_path);
		return false;
	}

	// Load package
	if (!packager::load_package(package_path, loaded_package))
	{
		log::error("Failed to load game package");
		return false;
	}

	log::info("Loaded package with {} entries", loaded_package.entries.size());

	// Build hash map for fast lookups
	// content_map.clear();
	for (const auto& entry : loaded_package.entries)
	{
		content_map[entry.relative_path] = &entry;
		log::info("Entry loaded: {}", entry.relative_path);
	}

	return true;
}

std::vector<std::byte> fileio::read_binary_file(const string& filename)
{
	// Check if file is in loaded package first (using wildcard path)
	auto it = content_map.find(filename);
	if (it != content_map.end())
	{
		const packager::package_entry* entry = it->second;
		return packager::decompress_entry(*entry);
	}

	// Not in package, try reading from disk (expand wildcards)
	const auto path = get_path(filename);
	ifstream file(path, ios::binary | ios::ate);
	if (!file.is_open())
	{
		log::error("File {} with full path {} was not found!", filename, path);
		return {};
	}

	const streamsize size = file.tellg();
	file.seekg(0, ios::beg);
	std::vector<std::byte> buffer(size);
	if (file.read((char*)buffer.data(), size))
		return buffer;

	return {};
}

string fileio::read_text_file(const string& filename)
{
	// Check if file is in loaded package first (using wildcard path)
	auto it = content_map.find(filename);
	if (it != content_map.end())
	{
		const packager::package_entry* entry = it->second;
		std::vector<std::byte> data = packager::decompress_entry(*entry);

		// Convert to string
		return string(reinterpret_cast<const char*>(data.data()), data.size());
	}

	// Not in package, try reading from disk (expand wildcards)
	const auto path = get_path(filename);
	ifstream file(path);
	if (!file.is_open())
	{
		log::error("File {} with full path {} was not found!", filename, path);
		return string();
	}

	file.seekg(0, ios::end);
	const size_t size = file.tellg();
	string buffer(size, '\0');
	file.seekg(0);
	file.read(&buffer[0], size);
	return buffer;
}

bool fileio::write_binary_file(const std::vector<std::byte>& data, const string& filename)
{
	auto fullpath = fileio::get_path(filename);
	ofstream ofs;
	ofs.open(fullpath);
	if (ofs.is_open())
	{
		ofs.write((char*)&data[0], data.size() * sizeof(char));;
		ofs.close();
		return true;
	}
	return false;
}

bool fileio::write_text_file(const string& text, const string& filename)
{
	auto fullpath = fileio::get_path(filename);
	ofstream ofs;
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

string fileio::get_path(const string& filename)
{
	string full_path = filename;

	for (const auto& p : wildcards)
	{
		if (full_path.find(p.first) != string::npos)
			full_path = tools::string_replace(full_path, p.first, p.second);
	}

	return full_path;
}

std::string xs::fileio::absolute(const std::string& path)
{
#if defined(PLATFORM_PC) || defined(PLATFORM_MAC)
	return fs::absolute(get_path(path)).string();
#else
	return path; // TODO: Implement for other platforms
#endif
}

bool fileio::exists(const string& filename)
{
	// Expand wildcards
	const auto path = get_path(filename);

	// Check if the file is stored in the package
	if(content_map.find(filename) != content_map.end())
		return true;

	// Check if the file exists
	ifstream f(path.c_str());
    auto good = f.good();
	f.close();
	return good;
}

#if (defined(PLATFORM_PC) || defined(PLATFORM_MAC))
void fileio::commit() {}
#endif

#if (defined(PLATFORM_PC) || defined(PLATFORM_MAC)) && (defined(XS_DEBUG) || defined(PROFILE))

uint64_t fileio::last_write(const string& filename)
{
	if(xs::get_run_mode() == xs::run_mode::packaged)
		return 0;

	// Expand wildcards
	const auto path = get_path(filename);
	fs::file_time_type ftime = fs::last_write_time(path);
	return static_cast<uint64_t>(ftime.time_since_epoch().count());
}

#else

uint64_t fileio::last_write(const string& filename) { return 0; }

#endif

bool xs::fileio::has_wildcard(const string& wildcard)
{
	return internal::wildcards.find(wildcard) != internal::wildcards.end();
}

void fileio::internal::load_project_settings()
{
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
}

#if !defined(PLATFORM_SWITCH)

// Default resolve_package_path: just return the path as-is
// macOS overrides this in fileio_apple.mm to try bundle-relative paths
#if !defined(PLATFORM_MAC)
string fileio::internal::resolve_package_path(const string& game_path)
{
	return game_path;
}
#endif

void fileio::initialize(const std::string& game_path)
{
	// [user]
	auto user_path = internal::get_user_path();
	if (!fs::exists(user_path))
		fs::create_directories(user_path);
	add_wildcard("[user]", user_path);

	// [save]
	auto save_path = (fs::path(user_path) / "save").string();
	if (!fs::exists(save_path))
		fs::create_directories(save_path);
	add_wildcard("[save]", save_path);

	// [game]
	bool success = false;
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
			auto package_path = internal::resolve_package_path(game_path);
			success = fileio::load_package(package_path);
		}
	}

	if (!success)
		log::warn("No game path resolved.");

	// [main] from project.json
	internal::load_project_settings();

	// [shared]
	auto shared = internal::get_shared_path();
	if (!shared.empty())
		add_wildcard("[shared]", shared);
}

#endif // !PLATFORM_NX
