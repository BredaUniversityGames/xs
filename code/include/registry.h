#pragma once
#include <string>

namespace xs::registry
{
	void initialize();
	void shutdown();
	void inspect();
	double get_number(const std::string& name);
	uint32_t get_color(const std::string& name);
}
