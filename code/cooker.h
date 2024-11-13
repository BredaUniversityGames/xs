#pragma once

#include <string>
#include <vector>

namespace xs
{
	namespace cooker
	{
		/*
		* Cook Content
		*
		* This function initiates the cooking process for content by generating and compressing an archive.
		* The generated archive is then written to disk. This is a key step in preparing content for runtime use.
		*
		* @param root - The root directory containing the content to be cooked.
		* @param sub_dirs - A vector of subdirectories within the root to include in the cooking process.
		* 
		* @return True if the cooking process is successful, false otherwise.
		*/
		bool cook_content(const std::string& root, const std::vector<std::string>& sub_dirs);
	}
}