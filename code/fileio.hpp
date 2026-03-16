#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
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

	namespace internal
	{
		extern std::map<std::string, std::string> wildcards;

		// Platform-specific: implemented per desktop platform (fileio_<platform>)
		std::string get_user_path();
		std::string get_shared_path();

		// Default for Linux/PC in fileio.cpp, macOS overrides in fileio_apple.mm
		std::string resolve_package_path(const std::string& game_path);

		// Common: implemented in fileio.cpp, used by all platforms
		void load_project_settings();
	}
}
