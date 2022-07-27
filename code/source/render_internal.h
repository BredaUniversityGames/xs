#pragma once

#include "render.h"
#include <glm/glm.hpp>
#include <vector>

namespace xs::render::internal
{
	struct sprite { int image_id; glm::vec2 from; glm::vec2 to; };
	struct sprite_queue_entry
	{
		int sprite_id = -1;
		double x = 0.0;
		double y = 0.0;
		double scale = 1.0;
		double rotation = 0.0;
		color mul_color = {};
		color add_color = {};
		unsigned int flags = 0;
	};

	std::vector<sprite_queue_entry>	sprite_queue = {};
	std::vector<sprite>				sprites = {};

	glm::vec4 to_vec4(color c) { return glm::vec4(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f); }
	void rotate_vector3d(glm::vec3& vec, float radians);
}

inline void xs::render::internal::rotate_vector3d(glm::vec3& vec, float radians)
{
	const float x = cos(radians) * vec.x - sin(radians) * vec.y;
	const float y = sin(radians) * vec.x + cos(radians) * vec.y;
	vec.x = x;
	vec.y = y;
}

