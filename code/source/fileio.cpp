#include "fileio.h"

#include <cassert>
#include <fstream>
#include <map>
#include <unordered_map>

#include "log.h"
#include "tools.h"
#include "resource_pipeline.h"
#include "miniz.h"

#if defined(PLATFORM_SWITCH)
#include <nn/fs.h>
#include <nn/nn_Assert.h>
#include <nn/nn_Abort.h>
#include <nn/nn_Log.h>
#include <nn/account/account_ApiForApplications.h>
#include "account_switch.h"
#elif defined(PLATFORM_PC)
#include <filesystem>
#elif defined(PLATFORM_APPLE)
#import <Foundation/Foundation.h>
#endif

namespace fs = std::filesystem;

using namespace std;

namespace xs::fileio::internal
{
	map<string, string> wildcards;

	unordered_map<std::string, resource_pipeline::content_header> text_content_headers;
	unordered_map<std::string, resource_pipeline::content_header> binary_content_headers;

	// ------------------------------------------------------------------------
	// Read data from a specific source and return the amount of bytes that are read
	size_t read_from_archive(void* dst, const void* src, size_t size)
	{
		memcpy(dst, src, size);
		return size;
	}

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
		return resource_pipeline::make_archive_path(fileio::get_path("[games]"), { game_str });
	}

	// ------------------------------------------------------------------------
	// Load game content from a file
	blob load_game_content()
	{
		// Create archive path from game content
		string archive_path = game_content_path();

		log::info("Loading archive: {}", archive_path);

		// Check if the archive exists
		if (fileio::exists(archive_path))
			// Read binary content from the created archive path
			return fileio::read_binary_file(archive_path);
			
		return {};
	}
	// ------------------------------------------------------------------------
	// Load compressed game archive
	void load_game_content_headers()
	{
		// Load game content from file
		blob game_content = load_game_content();

		if (game_content.empty())
		{
			log::info("Game archive not found loading.");
			return;
		}

		log::info("Read {0} bytes archive from disk", game_content.size());

		size_t entries_count = 0;
		size_t offset = read_from_archive(&entries_count, game_content.data(), sizeof(size_t));

		log::info("Loading {0} entries", entries_count);

		// Load in all archives from game data blob
		for (int i = 0; i < entries_count; ++i)
		{
			resource_pipeline::content_header header;
			offset += read_from_archive(&header, game_content.data() + offset, sizeof(resource_pipeline::content_header));
			auto path = get_path(header.file_path);


			if (header.file_size_compressed != 0)
			{
				log::info("Text entry loaded: {0}", header.file_path);
				text_content_headers.emplace(path, header);
				// Offset with the content data itself so we can jump to the next content_header
				offset += header.file_size_compressed; 
			}
			else
			{
				log::info("Binary entry loaded: {0}", header.file_path);
				binary_content_headers.emplace(path, header);
				// Offset with the content data itself so we can jump to the next content_header
				offset += header.file_size; 
			}
		}
	}
}

using namespace xs;
using namespace fileio::internal;

blob fileio::read_binary_file(const string& filename)
{
	const auto path = get_path(filename);

	bool has_binary_content = binary_content_headers.empty() == false;
	bool has_given_content_file = binary_content_headers.find(path) != binary_content_headers.cend();

	if (has_binary_content && has_given_content_file)
	{
		// Open the content archive
		std::string game_content_file_path = game_content_path();
		ifstream file(game_content_path(), ios::binary | ios::ate);
		if (!file.is_open())
		{
			log::error("File {} with full path {} was not found!", game_content_file_path, path);
			return {};
		}

		const resource_pipeline::content_header& header = binary_content_headers.at(path);
		file.seekg(header.file_offset, ios::beg);
		blob buffer(header.file_size);
		if (file.read((char*)buffer.data(), header.file_size))
			return buffer;
	}
	else
	{
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
	}

	assert(false);
	return {};
}

string fileio::read_text_file(const string& filename)
{
	const auto path = get_path(filename);

	bool has_text_content = text_content_headers.empty() == false;
	bool has_given_content_file = text_content_headers.find(path) != text_content_headers.cend();

	if (has_text_content && has_given_content_file)
	{
		// Open the content archive
		std::string game_content_file_path = game_content_path();
		ifstream file(game_content_path(), ios::binary | ios::ate);
		if (!file.is_open())
		{
			log::error("File {} with full path {} was not found!", game_content_file_path, path);
			return {};
		}

		const resource_pipeline::content_header& header = text_content_headers.at(path);

		file.seekg(header.file_offset, ios::beg);
		blob compressed_buffer(header.file_size_compressed);
		if (file.read((char*)compressed_buffer.data(), header.file_size_compressed))
		{
			unsigned long size = (unsigned long)header.file_size;
			string buffer(size, '\0');

			// Decompress using miniz library
			int dcmp_status = uncompress((unsigned char*)buffer.data(), &size, (const unsigned char*)(compressed_buffer.data()), (mz_ulong)(header.file_size_compressed));
			if (dcmp_status != Z_OK)
			{
				log::error("uncompress failed!");
				return {};
			}

			return buffer;
		}
	}
	else
	{
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

	assert(false);
	return {};
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
	return false;   // TODO: Warning?
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
	if (binary_content_headers.find(path) != binary_content_headers.cend() || text_content_headers.find(path) != text_content_headers.cend())
		return true;

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
