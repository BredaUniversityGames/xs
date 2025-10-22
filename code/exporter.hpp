#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "types.hpp"

namespace xs
{
	// ------------------------------------------------------------------------
	// Archive Format - Cross-platform serialization structures
	// ------------------------------------------------------------------------
	namespace archive_v2
	{
		struct ContentEntry
		{
			std::string relative_path;       // Relative path from content root
			uint64_t uncompressed_size;      // Original file size
			std::vector<std::byte> data;     // File data (compressed if is_compressed=true)
			bool is_compressed;              // Whether data is zlib compressed

			template<class Archive>
			void serialize(Archive& ar)
			{
				ar(relative_path, uncompressed_size, data, is_compressed);
			}
		};

		struct ArchiveData
		{
			std::vector<ContentEntry> entries;

			template<class Archive>
			void serialize(Archive& ar)
			{
				ar(entries);
			}
		};

		// Decompress a content entry if it's compressed, return the raw data
		blob decompress_entry(const ContentEntry& entry);
	}
	namespace exporter
	{
		/*
		* Export Archive
		*
		* Simplified cross-platform archive export using cereal serialization.
		* Accepts multiple source directories from anywhere on disk and exports to a single archive.
		*
		* @param source_dirs - Vector of source directories to include in the archive.
		* @param output_path - The full path where the archive should be written.
		*
		* @return True if the export process is successful, false otherwise.
		*/
		bool export_archive(const std::vector<std::string>& source_dirs, const std::string& output_path);

		/*
		* Load Archive
		*
		* Load an archive created with export_archive() and return its contents.
		* Returns empty ArchiveData on failure.
		*
		* @param archive_path - Path to the archive file to load.
		* @param out_archive - Output parameter that will contain the loaded archive data.
		*
		* @return True if successful, false otherwise.
		*/
		bool load_archive(const std::string& archive_path, archive_v2::ArchiveData& out_archive);

		/*
		* Make Archive Path
		*
		* Generate a standard archive path from a root directory and optional subdirectories.
		* The archive name is derived from the subdirectories (excluding "shared").
		*
		* @param root - Root directory where the archive will be placed.
		* @param sub_dirs - Optional vector of subdirectory names to include in archive name.
		*
		* @return Full path to the archive file with .xs extension.
		*/
		std::string make_archive_path(const std::string& root, const std::vector<std::string>& sub_dirs = {});

		/*
		* Check if a file extension is a text file format that should be compressed.
		*
		* @param extension - File extension (e.g., ".wren", ".json")
		* @return True if the extension is a text format.
		*/
		bool is_text_file(const std::string& extension);

		/*
		* Check if a file extension is supported for archiving.
		*
		* @param extension - File extension (e.g., ".png", ".wren")
		* @return True if the extension is supported.
		*/
		bool is_supported_file_format(const std::string& extension);
	}
}