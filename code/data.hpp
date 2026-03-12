#pragma once
#include <cstdint>
#include <string>

namespace xs::data
{
	enum class type
	{
		none		= 1,
		project		= 2,
		debug		= 3,
		game		= 4,
		player		= 5,
		user		= 6,
	};

	void initialize();
	void shutdown();
	void inspect();
	bool has_chages();

	double get_number(const std::string& name, type type, double default_value = 0.0);
	uint32_t get_color(const std::string& name, type type, uint32_t default_value = 0);
	bool get_bool(const std::string& name, type type, bool default_value = false);
	std::string get_string(const std::string& name, type type, const std::string& default_value = "");

	void set_number(const std::string& name, double value, type tp);
	void set_color(const std::string& name, uint32_t value, type tp);
	void set_bool(const std::string& name, bool value, type tp);
	void set_string(const std::string& name, const std::string& value, type tp);

	void save();
	void save_of_type(type type);
}
