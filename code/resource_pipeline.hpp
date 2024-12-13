#pragma once

#include "types.hpp"

#include <string>
#include <vector>
#include <unordered_set>

namespace xs
{
	namespace resource_pipeline
	{
        // ------------------------------------------------------------------------
        // Maximum length for file paths
        constexpr short s_max_path = 260;

        // ------------------------------------------------------------------------
        // Structure representing the header information for content
        struct content_header
        {
            content_header();
            content_header(const std::string& path, unsigned long long file_offset, unsigned long long file_size, unsigned long long file_size_compressed = 0);

            char                    file_path[s_max_path];
            unsigned long long      file_offset;
            unsigned long long      file_size;
            unsigned long long      file_size_compressed;

            bool operator==(const content_header& other) const;
            bool operator!=(const content_header& other) const;
        };

        // Function to generate an archive path based on the root and subdirectories
        std::string make_archive_path(const std::string& root, const std::vector<std::string>& sub_dirs = {});

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

namespace std
{
    template <>
    struct hash<xs::resource_pipeline::content_header>
    {
        size_t operator()(const xs::resource_pipeline::content_header& header) const
        {
            return std::hash<std::string> {}(header.file_path);
        }
    };
}