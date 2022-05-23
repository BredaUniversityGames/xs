#include <tools.h>
#include <ios>
#include <sstream>

using namespace xs;

std::string xs::tools::string_replace(
	const std::string& subject,
	const std::string& search,
	const std::string& replace)
{
	std::string result(subject);
	size_t pos = 0;

	while ((pos = subject.find(search, pos)) != std::string::npos)
	{
		result.replace(pos, search.length(), replace);
		pos += search.length();
	}

	return result;
}

std::tuple<double, double, double, double> tools::parse_color(const std::string& hex_color)
{
	std::string color_str = hex_color;
	if (color_str.length() == 6)
		color_str.append("FF");

	std::stringstream ss;
	unsigned int temp;
	ss << std::hex << hex_color;
	ss >> temp;

	const unsigned int ir = (temp & 0xFF000000) >> 24;
	const unsigned int ig = (temp & 0x00FF0000) >> 16;
	const unsigned int ib = (temp & 0x0000FF00) >> 8;
	const unsigned int ia = temp & 0x000000FF;

	auto r = static_cast<double>(ir) / 255.0;
	auto g = static_cast<double>(ig) / 255.0;
	auto b = static_cast<double>(ib) / 255.0;
	auto a = static_cast<double>(ia) / 255.0;

	return { r, g, b, a };
}
