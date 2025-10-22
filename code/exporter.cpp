#include "exporter.hpp"
#include "fileio.hpp"
#include "log.hpp"
#include "types.hpp"
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

namespace archive_v2
{
	// Decompress a content entry if compressed, otherwise return data as-is
	blob decompress_entry(const ContentEntry& entry)
	{
		if (!entry.is_compressed)
		{
			// Return data as-is for uncompressed entries
			return entry.data;
		}

		// Decompress the data
		blob decompressed;
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
}

namespace exporter
{
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

		std::string make_archive_name(const std::vector<std::string>& sub_dirs)
		{
			// Names that should not be included in the archive name when exported
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

	std::string make_archive_path(const std::string& root, const std::vector<std::string>& sub_dirs)
	{
		return root + "/" + make_archive_name(sub_dirs) + ".xs";
	}

	// ------------------------------------------------------------------------
	// Archive Export - Cross-platform export using cereal
	// ------------------------------------------------------------------------

	bool export_archive(const std::vector<std::string>& source_dirs, const std::string& output_path)
	{
		archive_v2::ArchiveData archive;

		// Process each source directory
		for (const auto& source_dir_str : source_dirs)
		{
			fs::path source_dir = fs::path(source_dir_str);

			if (!fs::exists(source_dir))
			{
				log::error("Source directory does not exist: {}", source_dir_str);
				continue;
			}

			log::info("Processing directory: {}", source_dir_str);

			// Single-pass iteration: read and process all files
			for (const auto& entry : fs::recursive_directory_iterator(source_dir))
			{
				if (!fs::is_regular_file(entry))
					continue;

				std::string extension = entry.path().extension().string();
				if (!is_supported_file_format(extension))
					continue;

				archive_v2::ContentEntry content;
				content.relative_path = fs::relative(entry.path(), source_dir).string();
				content.uncompressed_size = entry.file_size();

				// Normalize path separators to forward slashes for cross-platform compatibility
				for (char& c : content.relative_path)
				{
					if (c == '\\')
						c = '/';
				}

				// Read file data
				blob file_data = fileio::read_binary_file(entry.path().string());

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

				archive.entries.push_back(std::move(content));
			}
		}

		// Write archive using cereal
		try
		{
			std::ofstream ofs(output_path, std::ios::binary);
			if (!ofs)
			{
				log::error("Failed to open output file: {}", output_path);
				return false;
			}

			cereal::BinaryOutputArchive cereal_archive(ofs);
			cereal_archive(archive);

			log::info("Successfully wrote archive with {} entries to: {}",
				archive.entries.size(), output_path);

			return true;
		}
		catch (const std::exception& e)
		{
			log::error("Failed to write archive: {}", e.what());
			return false;
		}
	}

	// ------------------------------------------------------------------------
	// Archive Load - Load cross-platform archives created with export_archive
	// ------------------------------------------------------------------------
	bool load_archive(const std::string& archive_path, archive_v2::ArchiveData& out_archive)
	{
		try
		{
			std::ifstream ifs(archive_path, std::ios::binary);
			if (!ifs)
			{
				log::error("Failed to open archive file: {}", archive_path);
				return false;
			}

			cereal::BinaryInputArchive cereal_archive(ifs);
			cereal_archive(out_archive);

			log::info("Successfully loaded archive with {} entries from: {}",
				out_archive.entries.size(), archive_path);

			return true;
		}
		catch (const std::exception& e)
		{
			log::error("Failed to load archive: {}", e.what());
			return false;
		}
	}
}

}
