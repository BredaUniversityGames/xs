#pragma once
#include <string>
#include <tuple>
#include <vector>
#include <limits>
#include <random>
#include <glm/glm.hpp>

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
	inline int hash_combine(const T& v)
	{
		std::hash<T> hasher;
		return (int)hasher(v);
	}

	template <typename T, typename... Rest>
	inline int hash_combine(const T& first, const Rest&... rest)
	{
		int seed = hash_combine(first);
		((seed ^= hash_combine(rest) + 0x9e3779b9 + (seed << 6) + (seed >> 2)), ...);
		return seed;
	}

	inline int random_id()
	{
		// random using C++11 random library
		static std::random_device rd;
		static std::mt19937 gen(rd());
		static std::uniform_int_distribution<int> dis(0, std::numeric_limits<int>::max());
		return dis(gen);
	}

	struct aabb
	{
		glm::vec2 min = glm::vec2(
			std::numeric_limits<float>::max(),
			std::numeric_limits<float>::max());
		glm::vec2 max = glm::vec2(
			std::numeric_limits<float>::min(),
			std::numeric_limits<float>::min());

		aabb() = default;

		aabb(const glm::vec2& min, const glm::vec2& max) : min(min), max(max) {}

		bool is_valid() const { return min.x <= max.x && min.y <= max.y; }

		void add_point(const glm::vec2& point) { min = glm::min(min, point); max = glm::max(max, point); }

		static bool overlap(const aabb& a, const aabb& b)
		{
			return a.min.x <= b.max.x && a.max.x >= b.min.x && a.min.y <= b.max.y && a.max.y >= b.min.y;
		}

		aabb transform(const glm::mat4& m) const
		{
			glm::vec2 p[4] = {
				glm::vec2(m * glm::vec4(min.x, min.y, 1.0f, 1.0f)),
				glm::vec2(m * glm::vec4(min.x, max.y, 1.0f, 1.0f)),
				glm::vec2(m * glm::vec4(max.x, max.y, 1.0f, 1.0f)),
				glm::vec2(m * glm::vec4(max.x, min.y, 1.0f, 1.0f))
			};
			aabb result(p[0], p[0]);
			for (int i = 1; i < 4; i++)
				result.add_point(p[i]);
			return result;
		}

		void debug_draw();		
	};

	struct handle
	{
		int id = 0;
		operator int() const { return id; }
		
	};
}
