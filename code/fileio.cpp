#include "fileio.hpp"

#include <cassert>
#include <fstream>
#include <map>
#include <unordered_map>

#include "log.hpp"
#include "tools.hpp"
#include "exporter.hpp"
#include "miniz.h"

#if defined(PLATFORM_PC)
#include <filesystem>
#endif

namespace fs = std::filesystem;

using namespace std;

namespace xs::fileio::internal
{
	map<string, string> wildcards;

	// Archive data - loaded once on startup
	archive_v2::ArchiveData loaded_archive;
	unordered_map<std::string, const archive_v2::ContentEntry*> content_map;

	// ------------------------------------------------------------------------
	std::string game_content_path()
	{
		// Read game content from a text file
		auto game_str = fileio::read_text_file("[games]/.ini");
		if (game_str.empty())
		{
			log::error("Cannot load game, .ini file seems to be empty");
			return {};
		}

		// Create archive path from game content
		return exporter::make_archive_path(fileio::get_path("[games]"), { game_str });
	}
	// ------------------------------------------------------------------------
	// Load game archive
	void load_game_content_headers()
	{
		std::string archive_path = game_content_path();

		if (!fileio::exists(archive_path))
		{
			log::info("Game archive not found: {}", archive_path);
			return;
		}

		// Load archive
		if (!exporter::load_archive(archive_path, loaded_archive))
		{
			log::error("Failed to load game archive");
			return;
		}

		log::info("Loaded archive with {} entries", loaded_archive.entries.size());

		// Build hash map for fast lookups
		content_map.clear();
		for (const auto& entry : loaded_archive.entries)
		{
			content_map[entry.relative_path] = &entry;
			log::info("Entry loaded: {}", entry.relative_path);
		}
	}
}

using namespace xs;
using namespace fileio::internal;

blob fileio::read_binary_file(const string& filename)
{
	const auto path = get_path(filename);

	// Check if file is in loaded archive
	auto it = content_map.find(path);
	if (it != content_map.end())
	{
		const archive_v2::ContentEntry* entry = it->second;
		return archive_v2::decompress_entry(*entry);
	}

	// Not in archive, try reading from disk
	ifstream file(path, ios::binary | ios::ate);
	if (!file.is_open())
	{
		log::error("File {} with full path {} was not found!", filename, path);
		return {};
	}

	const streamsize size = file.tellg();
	file.seekg(0, ios::beg);
	blob buffer(size);
	if (file.read((char*)buffer.data(), size))
		return buffer;

	return {};
}

string fileio::read_text_file(const string& filename)
{
	const auto path = get_path(filename);

	// Check if file is in loaded archive
	auto it = content_map.find(path);
	if (it != content_map.end())
	{
		const archive_v2::ContentEntry* entry = it->second;
		blob data = archive_v2::decompress_entry(*entry);

		// Convert blob to string
		return string(reinterpret_cast<const char*>(data.data()), data.size());
	}

	// Not in archive, try reading from disk
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

bool fileio::write_binary_file(const blob& blob, const string& filename)
{
	auto fullpath = fileio::get_path(filename);
	ofstream ofs;
	ofs.open(fullpath);
	if (ofs.is_open())
	{
		ofs.write((char*)&blob[0], blob.size() * sizeof(char));;
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
#if defined(PLATFORM_PC)
	return fs::absolute(get_path(path)).string();
#else
	return path; // TODO: Implement for other platforms
#endif
}

bool fileio::exists(const string& filename)
{
	// Expand wildcards
	const auto path = get_path(filename);

	// Check if the file is stored in the archive
	// TODO: re-enable this check when needed
	// if (binary_content_headers.find(path) != binary_content_headers.cend() || text_content_headers.find(path) != text_content_headers.cend())
	//	return true;

	// Check if the file exists
	ifstream f(path.c_str());
    auto good = f.good();
	f.close();
	return good;
}

#if (defined(PLATFORM_PC) || defined(PLATFORM_MAC)) && (defined(DEBUG) || defined(PROFILE))

uint64_t fileio::last_write(const string& filename)
{
	const auto path = get_path(filename);
	fs::file_time_type ftime = fs::last_write_time(path);
	return static_cast<uint64_t>(ftime.time_since_epoch().count());
}

#else 

uint64_t fileio::last_write(const string& filename) { return 0; }

#endif
