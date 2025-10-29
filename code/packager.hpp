#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace xs::packager
{
	// Cross-platform serialization structures
	struct package_entry
	{
		// Relative path from content root
		std::string relative_path;

		// Original file size before compression
		uint64_t uncompressed_size;

		// Offset in package file where data starts (relative to data section)
		uint64_t data_offset;

		// Length of data in package file
		uint64_t data_length;

		// Whether data is zlib compressed
		bool is_compressed;

		// File data (will be populated on load, may be removed later for lazy loading)
		std::vector<std::byte> data;

		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(relative_path, uncompressed_size, data_offset, data_length, is_compressed);
			// Note: data vector is NOT serialized - it will be loaded separately
		}
	};

	// Cross-platform serialization structure
	struct package
	{
		// Magic number to identify xs package files: 0x58534E47 = "XSNG" (XS eNGine)
		static constexpr uint32_t MAGIC_NUMBER = 0x58534E47;

		// Package format fields
		uint32_t magic = MAGIC_NUMBER;
		uint32_t version = 0;  // YY.BuildNumber encoded as single integer

		std::vector<package_entry> entries;

		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(magic, version, entries);
		}
	};

	// Decompress a package entry if it's compressed, return the raw data
	std::vector<std::byte> decompress_entry(const package_entry& entry);

	/*
	* Encode Version
	*
	* Encodes a version string (e.g., "25.120") into a uint32_t for storage in package files.
	* Format: (year << 16) | build_number
	*
	* @param version_string - Version in "YY.BuildNumber" format
	* @return Encoded version as uint32_t
	*/
	uint32_t encode_version(const std::string& version_string);

	/*
	* Decode Version
	*
	* Decodes a uint32_t version back to "YY.BuildNumber" format.
	*
	* @param encoded_version - Encoded version as uint32_t
	* @return Version string in "YY.BuildNumber" format
	*/
	std::string decode_version(uint32_t encoded_version);

	/*
	* Create Package
	*
	* Creates a cross-platform package using cereal serialization.
	* Packages all files from the [game] and [shared] wildcards (if defined).
	* Stores paths with wildcard prefixes (e.g., "[game]/script.wren").
	* Automatically filters dotfiles and hidden directories.
	*
	* @param output_path - The full path where the package should be written.
	*
	* @return True if the creation process is successful, false otherwise.
	*/
	bool create_package(const std::string& output_path);

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
	bool load_package(const std::string& package_path, package& out_package);

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
