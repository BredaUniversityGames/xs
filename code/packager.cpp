#include "packager.hpp"
#include "fileio.hpp"
#include "log.hpp"
#include "miniz.h"
#include <filesystem>
#include <fstream>
#include <functional>
#include <limits>
#include <sstream>
#include <unordered_set>
#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>

namespace xs
{

namespace fs = std::filesystem;

namespace packager
{
	// ------------------------------------------------------------------------
	// Decompress package entry if compressed, otherwise return data as-is
	// ------------------------------------------------------------------------
	std::vector<std::byte> decompress_entry(const package_entry& entry)
	{
		if (!entry.is_compressed)
		{
			// Return data as-is for uncompressed entries
			return entry.data;
		}

		// Decompress the data
		std::vector<std::byte> decompressed;
		decompressed.resize(entry.uncompressed_size);

		unsigned long decompressed_size = static_cast<unsigned long>(entry.uncompressed_size);
		int result = uncompress(
			reinterpret_cast<unsigned char*>(decompressed.data()), &decompressed_size,
			reinterpret_cast<const unsigned char*>(entry.data.data()), static_cast<unsigned long>(entry.data.size()));

		if (result != Z_OK)
		{
			log::error("Failed to decompress entry: {}", entry.relative_path);
			return {};
		}

		return decompressed;
	}

	// ------------------------------------------------------------------------
	// File Format Support
	// ------------------------------------------------------------------------

	namespace
	{
		const std::unordered_set<std::string>& supported_text_file_formats()
		{
			static std::unordered_set<std::string> file_formats =
			{
				".wren",		// scripting
				".frag",		// shaders
				".vert",		// shaders
				".json",		// text
				".txt"			// text
			};
			return file_formats;
		}

		const std::unordered_set<std::string>& supported_binary_file_formats()
		{
			static std::unordered_set<std::string> file_formats =
			{
				".ttf",			// fonts
				".otf",			// fonts
				".png",			// images
				".bank",		// audio
				".wav",			// audio
				".mp3"			// audio
			};
			return file_formats;
		}

		std::string make_package_name(const std::vector<std::string>& sub_dirs)
		{
			// Names that should not be included in the package name
			static std::unordered_set<std::string> blacklist = { "shared" };

			std::stringstream stream;

			for (int i = 0; i < sub_dirs.size(); ++i)
			{
				if (blacklist.find(sub_dirs[i]) == blacklist.end())
				{
					stream << sub_dirs[i];

					// If we are NOT processing the last element then add a "_"
					if (i != sub_dirs.size() - 1)
					{
						stream << "_";
					}
				}
			}

			return stream.str();
		}
	}

	bool is_text_file(const std::string& extension)
	{
		return supported_text_file_formats().find(extension) != supported_text_file_formats().end();
	}

	bool is_supported_file_format(const std::string& extension)
	{
		return supported_text_file_formats().find(extension) != supported_text_file_formats().end() ||
		       supported_binary_file_formats().find(extension) != supported_binary_file_formats().end();
	}

	std::string make_package_path(const std::string& root, const std::vector<std::string>& sub_dirs)
	{
		return root + "/" + make_package_name(sub_dirs) + ".xs";
	}

	// ------------------------------------------------------------------------
	// Package Creation - Cross-platform packaging using cereal
	// ------------------------------------------------------------------------

	namespace
	{
		bool should_skip_entry(const fs::directory_entry& entry)
		{
			std::string filename = entry.path().filename().string();

			// Skip hidden files and directories (starting with '.')
			if (!filename.empty() && filename[0] == '.')
				return true;

			// Skip non-regular files
			if (!fs::is_regular_file(entry))
				return true;

			return false;
		}

		bool should_skip_path(const fs::path& path)
		{
			// Check if any component of the path starts with '.'
			for (const auto& component : path)
			{
				std::string comp_str = component.string();
				if (!comp_str.empty() && comp_str[0] == '.')
					return true;
			}
			return false;
		}
	}

	bool create_package(const std::string& output_path)
	{
		package pkg;

		// Define which wildcards to package
		std::vector<std::string> wildcards_to_package = { "[game]", "[shared]" };

		for (const auto& wildcard : wildcards_to_package)
		{
			// Check if wildcard is defined
			if (!fileio::has_wildcard(wildcard))
			{
				log::warn("Wildcard {} is not defined, skipping", wildcard);
				continue;
			}

			std::string wildcard_path = fileio::get_path(wildcard);
			fs::path source_dir = fs::path(wildcard_path);

			if (!fs::exists(source_dir))
			{
				log::error("Wildcard {} path does not exist: {}", wildcard, wildcard_path);
				continue;
			}

			log::info("Packaging {}: {}", wildcard, wildcard_path);

			// Single-pass iteration: read and process all files
			for (const auto& entry : fs::recursive_directory_iterator(source_dir))
			{
				if (should_skip_entry(entry))
					continue;

				// Get relative path and check for hidden components
				fs::path rel_path = fs::relative(entry.path(), source_dir);
				if (should_skip_path(rel_path))
					continue;

				std::string extension = entry.path().extension().string();
				if (!is_supported_file_format(extension))
					continue;

				package_entry content;

				// Store path with wildcard prefix: "[game]/images/sprite.png"
				std::string rel_path_str = rel_path.string();
				// Normalize path separators to forward slashes
				for (char& c : rel_path_str)
				{
					if (c == '\\')
						c = '/';
				}
				content.relative_path = wildcard + "/" + rel_path_str;
				content.uncompressed_size = entry.file_size();

				// Read file data
				std::vector<std::byte> file_data = fileio::read_binary_file(entry.path().string());

				// Compress text files
				if (is_text_file(extension))
				{
					unsigned long src_len = static_cast<unsigned long>(file_data.size());
					unsigned long compressed_size = compressBound(src_len);

					content.data.resize(compressed_size);

					int result = compress(
						reinterpret_cast<unsigned char*>(content.data.data()), &compressed_size,
						reinterpret_cast<const unsigned char*>(file_data.data()), src_len);

					if (result != Z_OK)
					{
						log::error("Failed to compress {}", content.relative_path);
						continue;
					}

					content.data.resize(compressed_size);
					content.is_compressed = true;

					log::info("Packed (compressed): {} ({} -> {} bytes)",
						content.relative_path, src_len, compressed_size);
				}
				else
				{
					// Binary files are stored uncompressed
					content.data = std::move(file_data);
					content.is_compressed = false;

					log::info("Packed: {} ({} bytes)",
						content.relative_path, content.data.size());
				}

				pkg.entries.push_back(std::move(content));
			}
		}

		// Write package using cereal
		try
		{
			std::ofstream ofs(output_path, std::ios::binary);
			if (!ofs)
			{
				log::error("Failed to open output file: {}", output_path);
				return false;
			}

			cereal::BinaryOutputArchive cereal_archive(ofs);
			cereal_archive(pkg);

			log::info("Successfully wrote package with {} entries to: {}",
				pkg.entries.size(), output_path);

			return true;
		}
		catch (const std::exception& e)
		{
			log::error("Failed to write package: {}", e.what());
			return false;
		}
	}

	// ------------------------------------------------------------------------
	// Package Loading - Load cross-platform packages created with create_package
	// ------------------------------------------------------------------------
	bool load_package(const std::string& package_path, package& out_package)
	{
		try
		{
			std::ifstream ifs(package_path, std::ios::binary);
			if (!ifs)
			{
				log::error("Failed to open package file: {}", package_path);
				return false;
			}

			cereal::BinaryInputArchive cereal_archive(ifs);
			cereal_archive(out_package);

			log::info("Successfully loaded package with {} entries from: {}",
				out_package.entries.size(), package_path);

			return true;
		}
		catch (const std::exception& e)
		{
			log::error("Failed to load package: {}", e.what());
			return false;
		}
	}
}

}
