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
            ContentHeader(const std::string& path, u64 fileSize);

            char8 file_path[s_max_path];
            u64 file_size;
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

        // Function to get the set of supported file formats
        const std::unordered_set<std::string>& supported_file_formats();

        // Function to check if a file format is supported
        bool is_supported_file_format(const std::string& extension);
	}
}