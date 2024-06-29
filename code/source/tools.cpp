#include <tools.h>
#include <ios>
#include <sstream>
#include <iomanip>
#include <render.h>

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

std::vector<std::string> xs::tools::string_split(
	const std::string& source,
	const char* delimiter,
	bool keep_empty)
{
	std::vector<std::string> results;

	size_t prev = 0;
	size_t next = 0;

	while ((next = source.find_first_of(delimiter, prev)) != std::string::npos)
	{
		if (keep_empty || (next - prev != 0))
		{
			results.push_back(source.substr(prev, next - prev));
		}
		prev = next + 1;
	}

	if (prev < source.size())
	{
		results.push_back(source.substr(prev));
	}

	return results;
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

std::string tools::float_to_str_with_precision(float f, int precision)
{
    std::stringstream stream;
    stream << std::fixed << std::setprecision(precision) << f;
    return stream.str();
}

void xs::tools::aabb::debug_draw()
{
	xs::color c;
	c.a = 255;
	c.r = 255;
	c.g = 255;
	c.b = 255;
	xs::render::set_color(c);
	xs::render::line(min.x, min.y, max.x, min.y);
	xs::render::line(max.x, min.y, max.x, max.y);
	xs::render::line(max.x, max.y, min.x, max.y);
	xs::render::line(min.x, max.y, min.x, min.y);
}
