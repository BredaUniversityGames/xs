#pragma once

#include "types.h"

#include <string>
#include <vector>

namespace xs::fileio
{
	void initialize();
	Blob read_binary_file(const std::string& filename);
	std::string read_text_file(const std::string& filename);
	bool write_binary_file(const Blob& blob, const std::string& filename);
	bool write_text_file(const std::string& text, const std::string& filename);
	void add_wildcard(const std::string& wildcard, const std::string& value);
	std::string get_path(const std::string& filename);
	bool exists(const std::string& filename);
	uint64_t last_write(const std::string& filename);
}