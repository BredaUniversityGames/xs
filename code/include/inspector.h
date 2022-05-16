#pragma once

namespace xs::inspector
{
	void initialize();
	void shutdown();
	void render(float dt);
	bool paused();
}
