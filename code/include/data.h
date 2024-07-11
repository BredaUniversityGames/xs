#pragma once
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
	void inspect(bool& show);	
	bool has_chages();

	double get_number(const std::string& name, type type);
	uint32_t get_color(const std::string& name, type type);
	bool get_bool(const std::string& name, type type);
	std::string get_string(const std::string& name, type type);

	void set_number(const std::string& name, double value, type tp);
	void set_color(const std::string& name, uint32_t value, type tp);
	void set_bool(const std::string& name, bool value, type tp);
	void set_string(const std::string& name, const std::string& value, type tp);
}
