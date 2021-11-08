#pragma once

#include <string>
#include <vector>

namespace xs::fileio
{
	void initialize();
	std::vector<char> read_binary_file(const std::string& filename);
	std::string read_text_file(const std::string& filename);
	void add_wildcard(const std::string& wildcard, const std::string& value);
	std::string get_path(const std::string& filename);
}
