#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "types.hpp"

namespace xs::packager
{
	// ------------------------------------------------------------------------
	// Package Format - Cross-platform serialization structures
	// ------------------------------------------------------------------------

	struct PackageEntry
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

	struct Package
	{
		std::vector<PackageEntry> entries;

		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(entries);
		}
	};

	// Decompress a package entry if it's compressed, return the raw data
	blob decompress_entry(const PackageEntry& entry);

	/*
	* Create Package
	*
	* Simplified cross-platform package creation using cereal serialization.
	* Accepts multiple source directories from anywhere on disk and creates a single package.
	*
	* @param source_dirs - Vector of source directories to include in the package.
	* @param output_path - The full path where the package should be written.
	*
	* @return True if the creation process is successful, false otherwise.
	*/
	bool create_package(const std::vector<std::string>& source_dirs, const std::string& output_path);

	/*
	* Load Package
	*
	* Load a package created with create_package() and return its contents.
	* Returns empty Package on failure.
	*
	* @param package_path - Path to the package file to load.
	* @param out_package - Output parameter that will contain the loaded package data.
	*
	* @return True if successful, false otherwise.
	*/
	bool load_package(const std::string& package_path, Package& out_package);

	/*
	* Make Package Path
	*
	* Generate a standard package path from a root directory and optional subdirectories.
	* The package name is derived from the subdirectories (excluding "shared").
	*
	* @param root - Root directory where the package will be placed.
	* @param sub_dirs - Optional vector of subdirectory names to include in package name.
	*
	* @return Full path to the package file with .xs extension.
	*/
	std::string make_package_path(const std::string& root, const std::vector<std::string>& sub_dirs = {});

	/*
	* Check if a file extension is a text file format that should be compressed.
	*
	* @param extension - File extension (e.g., ".wren", ".json")
	* @return True if the extension is a text format.
	*/
	bool is_text_file(const std::string& extension);

	/*
	* Check if a file extension is supported for packaging.
	*
	* @param extension - File extension (e.g., ".png", ".wren")
	* @return True if the extension is supported.
	*/
	bool is_supported_file_format(const std::string& extension);
}
