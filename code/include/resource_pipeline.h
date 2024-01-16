#pragma once

#include "types.h"

#include <string>
#include <vector>
#include <unordered_set>

namespace xs
{
	namespace resource_pipeline
	{
        // ------------------------------------------------------------------------
        // Maximum length for file paths
        constexpr s16 s_max_path = 260;

        // ------------------------------------------------------------------------
        // Structure representing the header information for content
        struct ContentHeader
        {
            ContentHeader();
            ContentHeader(const std::string& path, u64 fileSize, char8 isText);

            char8 file_path[s_max_path];
            u64 file_size;
            char8 is_text;
        };

        // ------------------------------------------------------------------------
        // Structure representing a compressed archive
        struct CompressedArchive
        {
            CompressedArchive();

            operator bool() const;

            ulong src_len;
            ulong cmp_len;

            Blob data;
        };

        // ------------------------------------------------------------------------
        // Structure representing an archive
        struct Archive
        {
            Archive();
            Archive(ulong totalSize);

            operator bool() const;

            ulong src_len;

            Blob data;
        };

        // Function to generate an archive path based on the root and subdirectories
        std::string make_archive_path(const std::string& root, const std::vector<std::string>& subDirs = {});

        // Function to get the set of supported text file formats
        const std::unordered_set<std::string>& supported_text_file_formats();
        // Function to get the set of supported binary file formats
        const std::unordered_set<std::string>& supported_binary_file_formats();

        // Function to check if a file format is supported
        bool is_supported_file_format(const std::string& extension);

        // Function that checks if a file is a binary file
        bool is_binary_file(const std::string& extension);
        // Function that checks if a file is a text file
        bool is_text_file(const std::string& extension);
	}
}