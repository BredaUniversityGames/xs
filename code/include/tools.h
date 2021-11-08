#pragma once
#include <string>
#include <tuple>

namespace xs::tools
{

std::string string_replace(
	const std::string& subject,
	const std::string& search,
	const std::string& replace);

std::tuple<double, double, double, double> parse_color(const std::string& hex_color);

}
