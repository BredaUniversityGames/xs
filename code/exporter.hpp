#pragma once

#include <string>
#include <vector>

namespace xs
{
	namespace exporter
	{
		/*
		* Export Content
		*
		* This function initiates the export process for content by generating and compressing an archive.
		* The generated archive is then written to disk. This is a key step in preparing content for runtime use.
		*
		* @param root - The root directory containing the content to be exported.
		* @param sub_dirs - A vector of subdirectories within the root to include in the export process.
		*
		* @return True if the export process is successful, false otherwise.
		*/
		bool export_content(const std::string& root, const std::vector<std::string>& sub_dirs);

		/*
		* Export Content to Specific Path
		*
		* This function initiates the export process for content and writes the archive to a specified path.
		*
		* @param root - The root directory containing the content to be exported.
		* @param sub_dirs - A vector of subdirectories within the root to include in the export process.
		* @param output_path - The full path where the archive should be written.
		*
		* @return True if the export process is successful, false otherwise.
		*/
		bool export_content_to_path(const std::string& root, const std::vector<std::string>& sub_dirs, const std::string& output_path);
	}
}