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
	}
}