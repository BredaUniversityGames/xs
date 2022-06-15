#pragma once
#include <string>

namespace xs::registry
{
	enum class type
	{
		none		= 1,
		system		= 2,
		debug		= 3,
		game		= 4,
		player		= 5,
	};

	void initialize();
	void shutdown();
	void inspect();	

	double get_number(const std::string& name);
	uint32_t get_color(const std::string& name);
	bool get_bool(const std::string& name);
	std::string get_string(const std::string& name);

	void set_number(const std::string& name, double value, type tp);
	void set_color(const std::string& name, uint32_t value, type tp);
	void set_bool(const std::string& name, bool value, type tp);
	void set_string(const std::string& name, const std::string& value, type tp);
}
