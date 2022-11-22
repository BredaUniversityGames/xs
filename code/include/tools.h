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
static inline void switch_on_bit_flag(unsigned int& flags, unsigned int bit) { flags |= bit; }
static inline void switch_off_bit_flag(unsigned int& flags, unsigned int bit) { flags &= (~bit); }
static inline bool check_bit_flag(unsigned int flags, unsigned int bit) { return (flags & bit) == bit; }
static inline bool check_bit_flag_overlap(unsigned int flag0, unsigned int flag1) { return (flag0 & flag1) != 0; }
static inline float saturate(float f) { return (f < 0.0f) ? 0.0f : (f > 1.0f) ? 1.0f : f; }
static inline uint32_t f32_to_uint8(float val) { return (uint32_t)(saturate(val) * 255.0f + 0.5f); }

static inline uint32_t next_power_of_two(uint32_t val)
{ 
	// https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2

	unsigned int v = val; // compute the next highest power of 2 of 32-bit v

	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;

	return v;
}
}
