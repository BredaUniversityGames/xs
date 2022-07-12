#pragma once

#include "render.h"
#include <glm/glm.hpp>

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
}
