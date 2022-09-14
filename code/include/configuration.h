#pragma once
#include <string>

namespace xs::configuration
{
	int width();
	int height();
	std::string title();
	bool fullscreen(); //TODO: Not suppported yet
	int multiplier();
}
