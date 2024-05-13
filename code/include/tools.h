#pragma once
#include <string>
#include <tuple>
#include <vector>

namespace xs::tools
{
	std::string string_replace(const std::string& subject, const std::string& search, const std::string& replace);
	std::vector<std::string> string_split(const std::string& source, const char* delimiter = " ", bool keep_empty = false);
	std::tuple<double, double, double, double> parse_color(const std::string& hex_color);

	inline void switch_on_bit_flag(unsigned int& flags, unsigned int bit) { flags |= bit; }
	inline void switch_off_bit_flag(unsigned int& flags, unsigned int bit) { flags &= (~bit); }

	inline bool check_bit_flag(unsigned int flags, unsigned int bit) { return (flags & bit) == bit; }
	inline bool check_bit_flag_overlap(unsigned int flag0, unsigned int flag1) { return (flag0 & flag1) != 0; }

	inline float saturate(float f) { return (f < 0.0f) ? 0.0f : (f > 1.0f) ? 1.0f : f; }

	inline uint32_t f32_to_uint8(float val) { return (uint32_t)(saturate(val) * 255.0f + 0.5f); }
	inline uint32_t next_power_of_two(uint32_t val)
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

	template <typename T>
	inline std::size_t hash_combine(const T& v)
	{
		std::hash<T> hasher;
		return hasher(v);
	}

	template <typename T, typename... Rest>
	inline std::size_t hash_combine(const T& first, const Rest&... rest)
	{
		std::size_t seed = hash_combine(first);
		((seed ^= hash_combine(rest) + 0x9e3779b9 + (seed << 6) + (seed >> 2)), ...);
		return seed;
	}
}
