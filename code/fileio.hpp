#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace xs::fileio
{
	void initialize(const std::string& game_path = "");
	bool load_package(const std::string& package_path);
	std::vector<std::byte> read_binary_file(const std::string& filename);
	std::string read_text_file(const std::string& filename);
	bool write_binary_file(const std::vector<std::byte>& data, const std::string& filename);
	bool write_text_file(const std::string& text, const std::string& filename);
	void add_wildcard(const std::string& wildcard, const std::string& value);
	std::string get_path(const std::string& filename);
	std::string absolute(const std::string& path);
	bool exists(const std::string& filename);
	uint64_t last_write(const std::string& filename);
	void commit();

	// Query if a wildcard is defined without modifying the state
	bool has_wildcard(const std::string& wildcard);
}
