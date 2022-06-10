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

inline void switch_on_bit_flag(unsigned int& flags, unsigned int bit) { flags |= bit; }
inline void switch_off_bit_flag(unsigned int& flags, unsigned int bit) { flags &= (~bit); }
inline bool check_bit_flag(unsigned int flags, unsigned int bit) { return (flags & bit) == bit; }
inline bool check_bit_flag_overlap(unsigned int flag0, unsigned int flag1) { return (flag0 & flag1) != 0; }

}
